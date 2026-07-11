// main.cpp — PanPilot entry point (base spec §3: setup + task spawn only).
// M2: Thermometer Mode. SensorTask (core 0) reads the MLX90640, runs
// frame_analysis + thermal_model, and publishes a UiState snapshot; the UI loop
// (core 1) renders the home screen (temp/rate/trend) with the thermal view one
// tap away, and dumps readings over serial.
#if !defined(PANPILOT_SIM)

#include <Arduino.h>
#include <lvgl.h>
#include <algorithm>
#include <cstdio>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "app_config.h"
#include "board_pins.h"
#include "pan_types.h"
#include "core/app_state.h"
#include "core/thermal_model.h"
#include "core/guidance.h"
#include "core/presets.h"
#include "core/power.h"
#include "core/session.h"
#include "core/profiles.h"
#include "core/profilestore.h"
#include "core/battery.h"
#include "core/attention.h"
#include "core/compliance.h"
#include "core/multipan.h"
#include "core/recipe.h"
#include "core/control/interlocks.h"
#include "core/control/controller.h"
#include "core/control/actuator.h"
#include "core/control/autotune.h"
#include "core/settings.h"
#include "core/timezones.h"
#include "core/foodfeedback.h"
#include "hal/display.h"
#include "hal/buzzer.h"
#include "hal/storage.h"
#include "hal/i2c_bus.h"
#include "hal/power.h"
#include "hal/ota.h"
#include "hal/session_store.h"
#include "hal/mqtt_plug_actuator.h"
#include "hal/food_json.h"
#include "core/foodstore.h"
#include "ui/screen_lastcook.h"
#include "sensor/mlx90640_source.h"
#include "sensor/frame_analysis.h"
#include "ui/ui_root.h"
#include "net/net.h"
#include "net/ha.h"

namespace {

struct Snapshot {
  ThermalFrame frame;
  PanReading reading;
  UiState ui;
  bool sceneHot = false;        // scene meaningfully above ambient (§8)
  bool heatingDetected = false; // rising heat event -> wake (§8)
  bool has = false;
} g_snap;
SemaphoreHandle_t g_snap_mtx = nullptr;
volatile bool g_sensor_ok = false;
volatile bool g_idle = false;   // set by UI loop, read by SensorTask (cadence)

// Battery (M7) — polled in the UI loop; snapshotted into UiState by SensorTask.
BatteryState g_batt;
volatile bool g_pluginWarn = false;

// Backlight brightness level (Settings, Phase 2). 0/1/2; read by SensorTask
// into UiState and by the UI loop to pick the ACTIVE-state PWM.
volatile uint8_t g_bright = 2;

// NTP clock (Settings). g_tz indexes TIMEZONES; the clock is read in the UI loop.
volatile uint8_t g_tz = 0;
volatile bool g_timeValid = false;
volatile uint8_t g_clockH = 0, g_clockM = 0;

// Tap-to-lock ROI (§6.3). UI core requests; SensorTask applies to its analyzer.
volatile uint8_t g_roi_req = 0;   // 0 none, 1 lock, 2 clear
volatile float g_roi_px = 0, g_roi_py = 0;

// Session trace (M11): 1 Hz temperature samples during a cook, written to
// LittleFS at session end. Owned by SensorTask. The finished summary is STAGED
// here and stored by the loop core — NVS/LittleFS writes must not run on the
// sensor core (flash-cache suspension stalls both cores mid-control-tick).
int16_t g_trace[1800];           // tempC*10, up to 30 min
volatile int g_traceN = 0;
SessionSummary g_pendingSession;
volatile bool g_session_pending = false;

// Last user interaction (touch) for interlock S10 — written by the loop core
// from LVGL's inactivity clock, read by SensorTask.
volatile uint32_t g_lastTouchMs = 0;

// Load the newest session from LittleFS and populate the Last Cook screen.
// Must run on the UI core (touches LVGL).
void refresh_lastcook(bool useF) {
  uint32_t ids[20];
  int n = hal::sessions_list(ids, 20);
  if (n <= 0) { SessionSummary empty{}; ui::lastcook_update(empty, nullptr, 0, false, useF); return; }
  SessionSummary sum;
  static int16_t tr[1800];
  if (hal::session_summary(ids[0], sum)) {
    int tn = hal::session_trace(ids[0], tr, 1800);
    ui::lastcook_update(sum, tr, tn, true, useF);
  }
}

// Target owned by the logic layer; UI sends adjust/select commands (base spec
// §7.3). Guarded because SensorTask (core 0) reads it while UI callbacks (loop,
// core 1) write it.
Target g_target;
volatile uint8_t g_presetId = PRESET_GENERIC;
SemaphoreHandle_t g_target_mtx = nullptr;

void save_target();   // defined below

// Food selection (M12.5): pointer into the static seed table; guarded by the
// target mutex alongside g_target (a food sets a custom target band).
const FoodEntry* volatile g_food = nullptr;
volatile uint8_t g_batch = 0;

// Post-cook feedback (spec §2.7). The store lives on the UI/loop core (on_food,
// on_feedback touch it); SensorTask only reads g_foodFactor (the current food's
// personalization multiplier) and raises g_awaitFeedback on REMOVE.
panpilot::FeedbackStore g_feedback;
#if defined(HAS_FILESYSTEM)
FoodStore g_foodstore;             // custom foods from /foods.json (spec §2.7)
#endif
volatile float g_foodFactor = 1.0f;
volatile bool g_awaitFeedback = false;
const FoodEntry* volatile g_fbFood = nullptr;   // food being graded

// Multi-pan zone 2 (M12): independent secondary target + a ready-edge chirp.
volatile int g_target2 = 300;
volatile bool g_z2ReadyChirp = false;

// Zone-2 food timer (Phase 3): the second pan runs its own cook.
const FoodEntry* volatile g_food2 = nullptr;
volatile uint8_t g_batch2 = 0;
volatile float g_foodFactor2 = 1.0f;

void on_preset2(uint8_t id) {
  g_target2 = (preset(id).loF + preset(id).hiF) / 2;   // zone-2 target band center
}
void on_food2(int id) {
  if (id < 0) { g_food2 = nullptr; g_foodFactor2 = 1.0f; }
  else {
    const FoodEntry* e = &foodlib_entry(id);
    g_food2 = e;
    g_foodFactor2 = g_feedback.factorFor(e->name, e->variant);
    g_target2 = (e->panTargetF_lo + e->panTargetF_hi) / 2;
    g_batch2 = 0;
  }
}

// Recipe sequencer (M19). cmd: 0=start built-in, 1=stop, 2=acknowledge cue.
volatile bool g_recipe_start_req = false;
volatile bool g_recipe_active = false;
volatile bool g_recipe_touch = false;
volatile int  g_recipe_setpoint = 0;   // recipe HOLD setpoint (drives guidance)
// Fat-state clamp (roadmap §4.1.1): once a recipe's PREP adds a fat, hold the
// pan below the fat's smoke point for the rest of the program. 0 = no clamp.
volatile int  g_fatClampWarnF = 0;

// Autopilot / ASSIST (M14-M18, M21). No actuator armed => pure ADVISORY, and
// there is no code path here that can energize anything (roadmap §0.1).
HeatActuator* volatile g_actuator = nullptr;   // set only after the arming ceremony
volatile bool g_assist_armed = false;
volatile bool g_assist_stop = false;
volatile float g_assist_duty = 0;
#if defined(ENABLE_WIFI)
// The one actuator PanPilot can arm: the SSR box / watchdog plug over MQTT.
// Duty is time-proportioned; the box's own watchdog fails safe on comms loss.
// Topic matches hardware/panpilot_ssr_box.yaml (the box subscribes
// panpilot/ssr/duty/set and publishes its birth/LWT on panpilot/ssr/status).
hal::MqttPlugActuator g_plug("SSR box", CTRL_WINDOW_SSR_MS, ha::actuator_publish,
                             "panpilot/ssr/duty/set");
#endif
void on_stop() {                                  // big STOP bar (interlock S9)
  g_assist_stop = true; g_assist_armed = false;
  if (g_actuator) g_actuator->emergencyOff();
}

// PID gains (roadmap §3.2). Applied to the controller each tick; default to the
// hand-tuned values until an autotune (or a loaded set) replaces them.
volatile float g_kp = 0.02f, g_ki = 0.0008f, g_kd = 0.02f;
volatile bool g_gains_valid = false;
volatile bool g_persist_gains = false;
// Autotune handshake. UI core -> SensorTask via g_autotune_req (1 start, 2 save,
// 3 discard); SensorTask -> UI via g_autotune_state/prog and the g_at_* result.
volatile uint8_t g_autotune_req = 0;
volatile uint8_t g_autotune_state = 0;   // 0 idle, 1 running, 2 done
volatile uint8_t g_autotune_prog = 0;
volatile float g_at_kp = 0, g_at_ki = 0, g_at_kd = 0;
void on_autotune(uint8_t cmd) { g_autotune_req = (cmd == 0) ? 1 : (cmd == 1 ? 2 : 3); }
// Arming ceremony result (roadmap §3.3). cmd 0 = arm, 1 = STOP/disarm.
void on_assist(uint8_t cmd) {
  if (cmd == 0) {
#if defined(ENABLE_WIFI)
    // M14.5 arming gate: refuse until the box's retained status says online.
    if (!g_plug.isAlive()) {
      Serial.println("[assist] REFUSED - SSR box not online (status topic)");
      return;
    }
    g_assist_stop = false;
    g_actuator = &g_plug;      // interlocks still gate every tick before the PID
    g_assist_armed = true;
    Serial.println("[assist] ARMED - Autopilot has burner authority");
#else
    Serial.println("[assist] no actuator on this build - advisory only");
#endif
  } else {
    on_stop();
    Serial.println("[assist] STOP");
  }
}
void on_recipe(uint8_t cmd) {
  if (cmd == 0) g_recipe_start_req = true;
  else if (cmd == 1) { g_recipe_active = false; g_fatClampWarnF = 0; }
  else if (cmd == 2) g_recipe_touch = true;
}

void on_food(int id) {
  if (xSemaphoreTake(g_target_mtx, pdMS_TO_TICKS(50)) != pdTRUE) return;
  if (id < 0) { g_food = nullptr; g_foodFactor = 1.0f; }
  else {
    const FoodEntry* e = &foodlib_entry(id);
    g_food = e;
    g_foodFactor = g_feedback.factorFor(e->name, e->variant);  // personalization
    g_target.loF = e->panTargetF_lo; g_target.hiF = e->panTargetF_hi;
    g_target.warnF = std::min((float)e->panTargetF_hi + 100, ABS_MAX_TEMP_F);
    g_target.centerF = (e->panTargetF_lo + e->panTargetF_hi) / 2;
    g_presetId = PRESET_GENERIC;
    g_batch = 0;
    save_target();
  }
  xSemaphoreGive(g_target_mtx);
}

void save_target() {
  hal::storage_set_target((int)g_target.loF, (int)g_target.hiF,
                          (int)g_target.warnF, g_presetId);
}

// Learn Pan Mode state (base spec §7 Phase 1.5). 0=off, 1=recording, 2=done.
volatile uint8_t g_learn_phase = 0;
volatile uint32_t g_learn_start = 0;
volatile float g_learn_peak = 0;
volatile float g_learned_lag = LAG_MINUTES_DEFAULT;
volatile uint8_t g_learn_progress = 0;
volatile float g_applied_lag = LAG_MINUTES_DEFAULT;  // fed to guidance
ProfileStore g_profiles;                             // up to 8 named pans (Phase 3)

void persist_profiles() {
  hal::storage_set_profiles(g_profiles.blob(), g_profiles.blobBytes());
  hal::storage_set_active_profile(g_profiles.active());
}
void apply_active_lag() {
  const PanProfile* ap = g_profiles.activeProfile();
  g_applied_lag = ap ? ap->lagMinutes : LAG_MINUTES_DEFAULT;
}
// Pans picker (Phase 3). cmd 0 = activate idx, 1 = delete idx.
void on_profile(uint8_t cmd, int idx) {
  if (cmd == 0) g_profiles.setActive(idx);
  else if (cmd == 1) g_profiles.remove(idx);
  persist_profiles();
  apply_active_lag();
}

void on_learn(uint8_t cmd) {
  if (cmd == 1) {                       // primary: Start (off) or Save (done)
    if (g_learn_phase == 0) {
      g_learn_peak = 0; g_learn_progress = 0;
      g_learn_start = millis(); g_learn_phase = 1;
    } else if (g_learn_phase == 2) {
      char nm[16];
      std::snprintf(nm, sizeof(nm), "Pan %d", g_profiles.count() + 1);
      g_profiles.add(make_profile(nm, g_learn_peak));   // appends + activates
      persist_profiles();
      apply_active_lag();
      g_learn_phase = 0;
    }
  } else if (cmd == 2) {                 // secondary: Cancel / Redo
    if (g_learn_phase == 2) {
      g_learn_peak = 0; g_learn_progress = 0;
      g_learn_start = millis(); g_learn_phase = 1;   // redo
    } else {
      g_learn_phase = 0;                 // cancel
    }
  }
}

void SensorTask(void*) {
  FrameAnalyzer analyzer;
  ThermalModel model;
  GuidanceEngine guidance;
  SessionAccumulator session;
  RecoveryMonitor recovery;
  Target target;
  ThermalFrame frame;
  float prevSceneMax = -1000.0f;
  int risingCount = 0;
  uint32_t coolStart = 0;
  uint32_t addBatchUntil = 0;
  uint32_t lastTraceMs = 0;
  FoodTimer foodtimer;
  const FoodEntry* prevFood = nullptr;
  uint32_t foodCueUntil = 0;
  const char* foodVerb = "";
  const char* foodSub = "";
  MultiPanTracker tracker;
  ThermalModel model2;
  GuidanceEngine guidance2;
  Target target2;
  GuidanceState prevZ2 = GuidanceState::IDLE;
  RecoveryMonitor recovery2;              // zone-2 food-added detection (Phase 3)
  FoodTimer foodtimer2;
  const FoodEntry* prevFood2 = nullptr;
  uint32_t foodCueUntil2 = 0;
  RecipeEngine recipe;
  InterlockMonitor interlocks;
  Controller controller;
  RelayAutotuner autotune;
  bool atActive = false;
  uint32_t lastCtrlMs = 0;
  uint32_t lastGoodFrameMs = 0;   // S6 frame-gap input + the read-fail failsafe
  uint32_t lastFoodMs = 0;        // last FOOD ADDED (counts as interaction, S10)
  for (;;) {
    if (!g_sensor_ok) {
      g_sensor_ok = sensor::mlx_begin();
      if (!g_sensor_ok) { vTaskDelay(pdMS_TO_TICKS(2000)); continue; }
      Serial.println("[PanPilot] MLX90640 online");
    }
    if (!sensor::mlx_read(frame)) {
      // SENSOR FAILSAFE: if frames stop while ASSIST is armed, the control
      // block below never runs — force duty to 0 (staged; the loop core
      // publishes). Without this the last nonzero duty stood until the box's
      // own watchdog. The box watchdog remains the independent backstop.
      if (g_assist_armed && g_actuator && lastGoodFrameMs != 0 &&
          millis() - lastGoodFrameMs > IL_FRAME_GAP_MS) {
        g_actuator->setDuty(0);
        g_assist_duty = 0;
      }
    } else {
      lastGoodFrameMs = millis();
      if (g_roi_req == 1) { analyzer.lockRoi(g_roi_px, g_roi_py); g_roi_req = 0; }
      else if (g_roi_req == 2) { analyzer.clearLock(); g_roi_req = 0; }
      PanReading r = analyzer.process(frame);
      model.update(r);
      uint8_t presetId = PRESET_GENERIC;
      if (xSemaphoreTake(g_target_mtx, pdMS_TO_TICKS(10)) == pdTRUE) {
        target = g_target; presetId = g_presetId;
        xSemaphoreGive(g_target_mtx);
      }
      // A running recipe's HOLD step drives the guidance target (M19), and a
      // fat-state clamp caps the overheat threshold so the pan can't be pushed
      // past the fat's smoke point (roadmap §4.1.1).
      if (g_recipe_active && g_recipe_setpoint > 0) target.setCenter(g_recipe_setpoint);
      if (g_recipe_active && g_fatClampWarnF > 0)
        target.warnF = std::min(target.warnF, (float)g_fatClampWarnF);

      GuidanceInput gi;
      gi.tempF = ThermalModel::cToF(model.displayTempC());
      gi.rateFPerMin = model.rateFPerMin();
      gi.confidence = r.confidence;
      gi.presence = r.presence;
      gi.moved = r.moved;
      gi.stainlessHint = r.stainlessHint;   // suppress reflective misread alarms
      gi.lagMinutes = g_applied_lag;    // learned lag (Learn Pan Mode)
      GuidanceOutput go = guidance.step(gi, target, millis());
      // Alerts are routed through the attention manager in the UI loop (§3.5).

      // Learn Pan Mode recording: capture the peak heating rate over the window.
      if (g_learn_phase == 1) {
        g_learn_peak = std::max((float)g_learn_peak, gi.rateFPerMin);
        const uint32_t el = millis() - g_learn_start;
        g_learn_progress = (uint8_t)std::min(100u, el * 100u / LEARN_DURATION_MS);
        if (el >= LEARN_DURATION_MS) {
          g_learned_lag = learn_lag_from_rate(g_learn_peak);
          g_learn_phase = 2;
        }
      }

      // Scene-hot + heating detection for power/wake (base spec §8).
      float sceneMax = frame.px[0][0];
      for (int rr = 0; rr < THERM_ROWS; ++rr)
        for (int cc = 0; cc < THERM_COLS; ++cc)
          sceneMax = std::max(sceneMax, frame.px[rr][cc]);
      const bool sceneHot = sceneMax > frame.ambientC + HEAT_WAKE_DELTA_C;
      const bool rising = sceneMax > prevSceneMax + 0.5f;
      risingCount = (sceneHot && rising) ? risingCount + 1 : 0;
      prevSceneMax = sceneMax;
      const bool heatingDetected = risingCount >= HEAT_WAKE_SAMPLES;

      const uint32_t nowm = millis();

      // Recovery Monitor (base spec §7.4): FOOD ADDED -> watch climb back.
      RecoveryOut ro = recovery.update(gi.tempF, gi.rateFPerMin,
                                       r.presence == PanPresence::PRESENT,
                                       target.loF, preset(presetId).recoveryMonitor,
                                       nowm);
      if (ro.event == RecoveryEvent::FOOD_ADDED) {
        session.foodAdded();
        lastFoodMs = nowm;             // food events count as interaction (S10)
        Serial.println("[recovery] food added");
      } else if (ro.event == RecoveryEvent::RECOVERED) {
        addBatchUntil = nowm + 4000;   // attention raises the ADD BATCH cue
        Serial.println("[recovery] pan recovered — add next batch");
      }
      const bool addBatch = nowm < addBatchUntil;

      // Food timer (M12.5): auto-start on FOOD ADDED; cue FLIP / REMOVE.
      const FoodEntry* food = g_food;
      if (food != prevFood) { foodtimer.stop(); prevFood = food; }
      FoodTimerOut fo;
      if (food) {
        if (ro.event == RecoveryEvent::FOOD_ADDED && !foodtimer.active())
          foodtimer.start(food, nowm, g_foodFactor);   // personalized (§2.7)
        fo = foodtimer.update(gi.tempF, nowm);
        if (fo.event == FoodTimerOut::FLIP) {
          foodCueUntil = nowm + 6000; foodVerb = "FLIP"; foodSub = food->flipHint;
          Serial.println("[food] flip");
        } else if (fo.event == FoodTimerOut::REMOVE) {
          foodCueUntil = nowm + 6000; foodVerb = "REMOVE";
          foodSub = food->safeInternalF ? "verify internal temp" : "";
          g_batch++;
          g_fbFood = food; g_awaitFeedback = true;   // ask Under/Perfect/Over (§2.7)
          Serial.println("[food] remove");
        }
      }
      const bool foodCue = nowm < foodCueUntil;

      // Multi-pan zone 2 (M12): independent guidance on a second burner. The
      // gate is zone-1-independent (zt[1] presence, not the pan count) so the
      // second cook survives the primary pan being lifted away, and debounced
      // over 3 frames so a momentary blob split can't flash the split screen.
      PanReading zt[2];
      tracker.process(frame, zt);
      const bool z2raw = zt[1].presence != PanPresence::ABSENT;
      static uint8_t z2on = 0, z2off = 0;
      static bool z2steady = false;
      if (z2raw) { z2off = 0; if (z2on < 255) ++z2on; }
      else       { z2on = 0;  if (z2off < 255) ++z2off; }
      if (!z2steady && z2on >= 3) z2steady = true;
      if (z2steady && z2off >= 3) z2steady = false;
      const bool z2 = z2steady && z2raw;
      GuidanceState z2g = GuidanceState::IDLE;
      float z2temp = 0;
      const FoodEntry* food2 = g_food2;
      FoodTimerOut fo2;
      if (z2) {
        model2.update(zt[1]);
        target2.setCenter(g_target2);
        GuidanceInput gi2;
        gi2.tempF = ThermalModel::cToF(model2.displayTempC());
        gi2.rateFPerMin = model2.rateFPerMin();
        gi2.confidence = zt[1].confidence;
        gi2.presence = zt[1].presence;
        gi2.moved = zt[1].moved;
        gi2.stainlessHint = zt[1].stainlessHint;   // zone-2 TOO_HOT suppression
        gi2.lagMinutes = g_applied_lag;
        z2g = guidance2.step(gi2, target2, nowm).state;
        z2temp = model2.displayTempC();
        if (z2g == GuidanceState::READY && prevZ2 != GuidanceState::READY)
          g_z2ReadyChirp = true;          // independent zone-2 READY alert
        prevZ2 = z2g;

        // Zone-2 food timer (Phase 3): own recovery detection + FLIP/REMOVE cues.
        RecoveryOut ro2 = recovery2.update(gi2.tempF, gi2.rateFPerMin,
                                           zt[1].presence == PanPresence::PRESENT,
                                           target2.loF, true, nowm);
        if (food2 != prevFood2) { foodtimer2.stop(); prevFood2 = food2; }
        if (food2) {
          if (ro2.event == RecoveryEvent::FOOD_ADDED && !foodtimer2.active())
            foodtimer2.start(food2, nowm, g_foodFactor2);
          fo2 = foodtimer2.update(gi2.tempF, nowm);
          if (fo2.event == FoodTimerOut::FLIP) {
            g_z2ReadyChirp = true; Serial.println("[food2] flip");
          } else if (fo2.event == FoodTimerOut::REMOVE) {
            g_batch2++; g_z2ReadyChirp = true; Serial.println("[food2] remove");
          }
        }
      } else {
        model2.reset(); guidance2.reset(); prevZ2 = GuidanceState::IDLE;
        foodtimer2.stop(); prevFood2 = nullptr;
      }

      // Recipe sequencer (M19): drive setpoints + cues from a cook program.
      if (g_recipe_start_req) {
        recipe.start(recipe_builtin_smashburger(), nowm);
        g_recipe_start_req = false; g_recipe_active = true; g_fatClampWarnF = 0;
        Serial.println("[recipe] started: Smash Burgers x4");
      }
      RecipeOut rout;
      if (g_recipe_active) {
        RecipeInput ri;
        ri.panTempF = gi.tempF;
        ri.rateFPerMin = gi.rateFPerMin;     // for fat equalize-detection (§4.1.1)
        ri.foodAdded = (ro.event == RecoveryEvent::FOOD_ADDED);
        ri.touch = g_recipe_touch; g_recipe_touch = false;
        ri.now = nowm;
        rout = recipe.step(ri);
        g_recipe_setpoint = rout.setpointF;
        // The engine latches the fat clamp for the whole program (min smoke
        // clamp across fats) and carries it on every tick's output.
        g_fatClampWarnF = rout.fatClampWarnF;
        if (rout.finished) { g_recipe_active = false; g_recipe_setpoint = 0;
          g_fatClampWarnF = 0;
          Serial.println("[recipe] finished"); }
      }

      // Autopilot / ASSIST closed loop (M14-M18). Runs ONLY with an armed
      // actuator; interlocks (§3.3) run before the PID and can only cut power.
      // The same recipe HOLD setpoints (above) drive it, so a recipe file runs
      // closed-loop unchanged on the SSR griddle (M21).
      // PID gains: apply the current set every tick (autotune / persisted).
      controller.setLaw(g_gains_valid ? ControlLaw::PID : ControlLaw::BANGBANG);
      controller.setGains(g_kp, g_ki, g_kd);

      // Autotune handshake (UI core -> here). Start only while armed; save copies
      // the converged gains out for the loop to persist; discard aborts.
      if (g_autotune_req == 1) {
        if (g_assist_armed && g_actuator) {
          autotune.start(target.centerF, 0.0f, 1.0f, 2.0f, nowm);
          atActive = true; g_autotune_state = 1; g_autotune_prog = 0;
          Serial.println("[autotune] started - pulsing burner");
        }
        g_autotune_req = 0;
      } else if (g_autotune_req == 2) {
        g_kp = g_at_kp; g_ki = g_at_ki; g_kd = g_at_kd;
        g_gains_valid = true; g_persist_gains = true;
        atActive = false; g_autotune_state = 0;
        Serial.println("[autotune] gains saved");
        g_autotune_req = 0;
      } else if (g_autotune_req == 3) {
        atActive = false; g_autotune_state = 0;
        g_autotune_req = 0;
      }
      if (!g_assist_armed && atActive) { atActive = false; g_autotune_state = 0; }

      float assistDuty = 0; int ilv = 0;
      if (g_assist_armed && g_actuator) {
        float dt = (nowm - lastCtrlMs) / 1000.0f; if (dt <= 0) dt = 0.25f;
        lastCtrlMs = nowm;
        InterlockInput ii;
        ii.confidence = r.confidence; ii.presence = r.presence;
        ii.duty = g_assist_duty; ii.rateFPerMin = gi.rateFPerMin;
        ii.tempF = gi.tempF; ii.warnF = target.warnF;
        ii.sensorFault = sensor::mlx_faulted(); ii.lastFrameMs = lastGoodFrameMs;
        ii.actuatorAlive = g_actuator->isAlive(); ii.lastAckMs = nowm;
#if defined(ENABLE_WIFI)
        ii.commsOk = ha::connected();   // broker link health (S8); box watchdog
                                        // remains the independent backstop
#else
        ii.commsOk = false;             // no comms path on this build (unarmable)
#endif
        ii.stopPressed = g_assist_stop;
        // S10 unattended: latest of last touch (loop core, LVGL inactivity
        // clock) and last FOOD ADDED event.
        ii.lastInteractionMs = std::max((uint32_t)g_lastTouchMs, lastFoodMs);
        ii.dieTempC = sensor::mlx_die_tempC(); ii.now = nowm;
        Interlock il = interlocks.evaluate(ii); ilv = (int)il;
        if (il == Interlock::NONE) {
          if (atActive) {               // relay autotune drives the actuator
            assistDuty = autotune.update(gi.tempF, nowm);
            g_autotune_prog = (uint8_t)autotune.cycles();
            if (autotune.converged()) {
              g_at_kp = autotune.kp(); g_at_ki = autotune.ki(); g_at_kd = autotune.kd();
              g_autotune_state = 2; atActive = false;
              Serial.printf("[autotune] converged Ku=%.3f Tu=%.1fs\n",
                            autotune.ku(), autotune.tuSeconds());
            } else if (!autotune.running()) {   // MAX_CYCLES runaway guard hit
              atActive = false; g_autotune_state = 0;
              Serial.println("[autotune] gave up - oscillation never settled");
            }
          } else {
            assistDuty = controller.update(gi.tempF, target.centerF, dt);
          }
        } else {
          if (atActive) {               // interlock -> abort any running autotune
            atActive = false; g_autotune_state = 0;   // (don't clobber a DONE result)
            Serial.println("[autotune] aborted by interlock");
          }
          if (InterlockMonitor::isEmergency(il)) { g_actuator->emergencyOff(); assistDuty = 0; }
          else assistDuty = 0;
        }
        g_actuator->setDuty(assistDuty);
        g_assist_duty = assistDuty;
      }

      // Session lifecycle (§8): start on a hot pan, end when cool + stable.
      // Records a 1 Hz temperature trace; the LOOP CORE stores it (NVS/LittleFS
      // writes stall flash cache and must not run on the sensor core). A new
      // session waits until the previous one has been flushed (~1 loop tick).
      if (!session.active() && !g_session_pending &&
          r.presence == PanPresence::PRESENT && sceneHot) {
        session.begin(presetId, nowm);
        g_traceN = 0; lastTraceMs = nowm;
      }
      if (session.active()) {
        session.update(r, go.state, nowm);
        if (nowm - lastTraceMs >= 1000 && g_traceN < (int)(sizeof(g_trace) / 2)) {
          lastTraceMs = nowm;
          g_trace[g_traceN++] = (int16_t)(model.displayTempC() * 10.0f);
        }
        if (r.panTempC < frame.ambientC + SESSION_END_AMBIENT_C) {
          if (coolStart == 0) coolStart = nowm;
          else if (nowm - coolStart > SESSION_END_STABLE_MS) {
            g_pendingSession = session.finish(nowm);
            g_session_pending = true;      // loop core stores + refreshes UI
            coolStart = 0;
            Serial.println("[session] finished - queued for store");
          }
        } else {
          coolStart = 0;
        }
      }

      UiState u;
      u.mode = Mode::TARGET;
      u.presence = r.presence;
      u.modelValid = model.valid();
      u.displayTempC = model.displayTempC();
      u.rateCPerMin = model.rateCPerMin();
      u.trend = model.trend();
      u.confidence = r.confidence;
      u.moved = r.moved;
      u.stainlessHint = r.stainlessHint;
      u.muted = hal::buzzer_is_muted();
      u.brightnessLevel = g_bright;
      u.tzIndex = g_tz;
      u.timeValid = g_timeValid;
      u.clockHour = g_clockH; u.clockMin = g_clockM;
      u.guidance = ro.recovering ? GuidanceState::RECOVERING : go.state;
      u.targetCenterF = target.centerF;
      u.presetId = presetId;
      u.etaSeconds = go.etaSeconds;
      u.projectedPeakF = go.projectedPeakF;
      u.recovering = ro.recovering;
      u.recoveryHint = ro.hint;
      u.addBatchPrompt = addBatch;
      u.food = food;
      u.foodTimer = fo;
      u.batchCount = g_batch;
      u.foodCue = foodCue;
      u.foodCueVerb = foodVerb;
      u.foodCueSub = foodSub;
      u.feedbackPrompt = g_awaitFeedback;
      u.feedbackName = g_fbFood ? g_fbFood->name : "";
      u.zone2Present = z2;
      u.zone2TempC = z2temp;
      u.zone2Guidance = z2g;
      u.zone2TargetF = g_target2;
      u.zone2Food = food2;
      u.zone2FoodTimer = fo2;
      u.zone2Batch = g_batch2;
      u.recipeActive = g_recipe_active;
      u.recipeCue = g_recipe_active ? rout.cue : "";
      u.recipeStepIndex = rout.stepIndex;
      u.assistArmed = g_assist_armed;
      u.assistDuty = assistDuty;
      u.assistInterlock = ilv;
      u.autotuneState = g_autotune_state;
      u.autotuneProgress = g_autotune_prog;
      u.autotuneKp = g_at_kp; u.autotuneKi = g_at_ki; u.autotuneKd = g_at_kd;
#if defined(ENABLE_WIFI)
      // Arming is offered only while the box's retained status says online
      // (M14.5) — not merely because the actuator code is compiled in.
      u.actuatorAvailable = g_plug.isAlive();
      u.actuatorName = g_plug.name();
#else
      u.actuatorAvailable = false;
      u.actuatorName = "";
#endif
      u.learnPhase = (LearnPhase)g_learn_phase;
      u.learnProgress = g_learn_progress;
      u.learnedLagMinutes = g_learned_lag;
      u.battery = g_batt;
      u.pluginWarning = g_pluginWarn;

      if (xSemaphoreTake(g_snap_mtx, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_snap.frame = frame; g_snap.reading = r; g_snap.ui = u;
        g_snap.sceneHot = sceneHot; g_snap.heatingDetected = heatingDetected;
        g_snap.has = true;
        xSemaphoreGive(g_snap_mtx);
      }
    }
    // Slow the sensor right down while idle to save power (base spec §8).
    vTaskDelay(pdMS_TO_TICKS(g_idle ? IDLE_SENSOR_PERIOD_MS
                                    : 1000 / MLX_REFRESH_HZ));
  }
}

void persist_unit(bool useF) { hal::storage_set_unit_useF(useF); }

// First-boot wizard finished (base spec §4 first-run) — don't show it again.
void on_onboarding_done() { hal::storage_set_onboarded(true); }

// Tap-to-lock ROI (§6.3). g_roi_* declared above; SensorTask applies them.
void on_roi(float px, float py, bool lock) {
  if (lock) { g_roi_px = px; g_roi_py = py; g_roi_req = 1; }
  else g_roi_req = 2;
}

void on_target_delta(int deltaF) {
  if (xSemaphoreTake(g_target_mtx, pdMS_TO_TICKS(50)) != pdTRUE) return;
  g_target.loF += deltaF; g_target.hiF += deltaF;
  g_target.warnF = std::min(g_target.warnF + deltaF, ABS_MAX_TEMP_F);
  g_target.centerF = (int)((g_target.loF + g_target.hiF) / 2);
  g_presetId = PRESET_GENERIC;    // an adjusted band is "custom"/generic
  save_target();
  xSemaphoreGive(g_target_mtx);
}

void on_preset(uint8_t id) {
  if (xSemaphoreTake(g_target_mtx, pdMS_TO_TICKS(50)) != pdTRUE) return;
  g_target = preset_target(id);
  g_presetId = id;
  save_target();
  xSemaphoreGive(g_target_mtx);
}

// Preset editor (Phase 2). Custom presets live on the UI/loop core; the sensor
// task only reads preset(id) by value, so mutation here is single-core safe.
void persist_custom_presets() {
  hal::storage_set_custom_presets(presets_custom_blob(), presets_custom_blob_bytes());
}
void on_preset_save(int editId, const char* name, int loF, int hiF, bool stainless) {
  if (editId < 0) presets_add(name, loF, hiF, stainless);
  else presets_update((uint8_t)editId, name, loF, hiF, stainless);
  persist_custom_presets();
}
void on_preset_delete(int id) {
  presets_remove((uint8_t)id);
  persist_custom_presets();
  // Custom ids above the deleted one shift down by one — remap the active
  // preset id (it is persisted, so a stale id survives reboots otherwise and
  // the home label / recovery-monitor flag silently follow the wrong preset).
  if (xSemaphoreTake(g_target_mtx, pdMS_TO_TICKS(50)) == pdTRUE) {
    if ((int)g_presetId == id) g_presetId = PRESET_GENERIC;  // band stays as-is
    else if ((int)g_presetId > id && g_presetId >= PRESET_COUNT) --g_presetId;
    save_target();
    xSemaphoreGive(g_target_mtx);
  }
}

// Absolute target set (used by the HA "target" number entity, M9).
void on_target_abs(int centerF) {
  if (xSemaphoreTake(g_target_mtx, pdMS_TO_TICKS(50)) != pdTRUE) return;
  g_target.setCenter(centerF);
  g_presetId = PRESET_GENERIC;
  save_target();
  xSemaphoreGive(g_target_mtx);
}

void on_mute(bool m) {
  hal::buzzer_set_muted(m);
  hal::storage_set_muted(m);
}

// Backlight brightness (Settings, Phase 2). g_bright is declared above; the
// loop reads it to pick the ACTIVE-state PWM, so a dim setting sticks to IDLE.
void on_brightness(uint8_t level) {
  g_bright = panpilot::brightness_clamp(level);
  hal::storage_set_brightness(g_bright);
  hal::display_set_brightness(panpilot::brightness_pwm(g_bright));
}

// Web settings mirror (Phase 2): the browser sets the unit through the same
// path as the on-device toggle (updates the UI + persists). Runs on the loop
// core via net::loop, so touching ui/NVS here is safe.
void web_set_unit(bool useF) { ui::set_display_unit(useF); }

// Timezone (Settings NTP clock). Persist + re-point SNTP at the new zone; the
// POSIX TZ string carries DST rules so the displayed time follows it.
void on_timezone(uint8_t idx) {
  g_tz = (uint8_t)tz_clamp(idx);
  hal::storage_set_timezone(g_tz);
#if defined(ENABLE_WIFI)
  configTzTime(tz_posix(g_tz), "pool.ntp.org", "time.nist.gov");
#endif
}

// Post-cook grade (spec §2.7): nudge the just-cooked food's timer ±8%, persist
// the table, refresh the live factor, and dismiss the prompt.
void on_feedback(uint8_t verdict) {
  const FoodEntry* f = g_fbFood;
  if (f) {
    g_feedback.apply(f->name, f->variant, (panpilot::FeedbackStore::Verdict)verdict);
    hal::storage_set_foodfb(g_feedback.blob(), g_feedback.blobBytes());
    if (g_food == f) g_foodFactor = g_feedback.factorFor(f->name, f->variant);
    Serial.printf("[feedback] %s graded %u -> factor %.3f\n", f->name, verdict,
                  g_feedback.factorFor(f->name, f->variant));
  }
  g_awaitFeedback = false;
  g_fbFood = nullptr;
}

// ACTIVE-state backlight: the user's level, dimmed no brighter than the
// battery cap when unplugged (roadmap §2.1).
uint8_t active_brightness() {
  uint8_t b = panpilot::brightness_pwm(g_bright);
  if (g_batt.valid && !g_batt.usbPresent && b > BACKLIGHT_BATTERY_BRIGHT)
    b = BACKLIGHT_BATTERY_BRIGHT;
  return b;
}

const char* presence_str(PanPresence p) {
  switch (p) {
    case PanPresence::PRESENT: return "PRESENT";
    case PanPresence::ABSENT: return "ABSENT";
    case PanPresence::OBSTRUCTED: return "OBSTRUCTED";
    default: return "UNCERTAIN";
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.printf("\n[PanPilot] %s  fw=%s\n", BOARD_NAME, PANPILOT_FW_VERSION);

  hal::ota_boot_guard();   // boot-loop rollback guard, before anything can hang

  hal::i2c_bus_init();
  hal::storage_begin();
  hal::sessions_begin();
  hal::power_begin();
  hal::display_begin();
  hal::buzzer_begin();
  hal::buzzer_set_muted(hal::storage_get_muted());

  g_target_mtx = xSemaphoreCreateMutex();
  { int lo, hi, warn, pid;
    hal::storage_get_target(lo, hi, warn, pid);
    g_target.loF = lo; g_target.hiF = hi; g_target.warnF = warn;
    g_target.centerF = (lo + hi) / 2; g_presetId = (uint8_t)pid; }
  { static uint8_t pbuf[ProfileStore::MAX * sizeof(PanProfile)];
    uint32_t n = hal::storage_get_profiles(pbuf, sizeof(pbuf));
    if (n > 0) {
      g_profiles.loadBlob(pbuf, n, hal::storage_get_active_profile(0));
    } else {
      PanProfile pf;                              // migrate a single old profile
      if (hal::storage_load_profile(pf)) g_profiles.add(pf);
    }
    apply_active_lag();
    Serial.printf("[profile] %d pan(s), active lag=%.2f min\n",
                  g_profiles.count(), g_applied_lag); }
#if defined(HAS_FILESYSTEM)
  if (hal::load_custom_foods(g_foodstore) > 0) foodlib_set_custom(&g_foodstore);
#endif
  g_bright = panpilot::brightness_clamp(hal::storage_get_brightness());
  g_tz = (uint8_t)tz_clamp(hal::storage_get_timezone());
  { uint8_t fbBlob[panpilot::FeedbackStore::MAX * 8];
    uint32_t n = hal::storage_get_foodfb(fbBlob, sizeof(fbBlob));
    if (n > 0) { g_feedback.loadBlob(fbBlob, n);
      Serial.printf("[feedback] loaded %d food(s)\n", g_feedback.count()); } }
  { uint8_t cp[PRESET_CUSTOM_MAX * 32];
    uint32_t n = hal::storage_get_custom_presets(cp, sizeof(cp));
    if (n > 0) { presets_load_custom(cp, n);
      Serial.printf("[presets] loaded %d custom\n", presets_custom_count()); } }
  ui::root_init(hal::storage_get_unit_useF(), persist_unit, on_target_delta,
                on_preset, on_learn, on_food, on_preset2, on_recipe);
  ui::set_settings_cbs(on_mute, on_brightness, on_timezone);
  ui::set_food2_cb(on_food2);
  ui::set_feedback_cb(on_feedback);
  ui::set_preset_edit_cbs(on_preset_save, on_preset_delete);
  ui::set_assist_cb(on_assist);
  ui::set_autotune_cb(on_autotune);
  { float kp, ki, kd;
    if (hal::storage_get_gains(kp, ki, kd)) {
      g_kp = kp; g_ki = ki; g_kd = kd; g_gains_valid = true;
      Serial.printf("[pid] loaded gains kp=%.3f ki=%.4f kd=%.3f\n", kp, ki, kd); } }
  ui::set_onboarding_cb(on_onboarding_done);
  ui::set_roi_cb(on_roi);
  ui::set_profiles(&g_profiles, on_profile);
  if (!hal::storage_get_onboarded()) ui::show_onboarding();   // first boot only

  refresh_lastcook(hal::storage_get_unit_useF());   // populate Last Cook at boot

  // LVGL heap check (screens are lazy-created; the worst case grows as screens
  // are visited). Bench-watch this line — free_biggest shrinking toward zero
  // means LV_MEM_SIZE needs raising for this board.
  { lv_mem_monitor_t m;
    lv_mem_monitor(&m);
    Serial.printf("[lvgl] heap used %u%% frag %u%% biggest %u\n",
                  m.used_pct, m.frag_pct, (unsigned)m.free_biggest_size); }

  g_snap_mtx = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(SensorTask, "sensor", 8192, nullptr, 2, nullptr, 0);

#if defined(ENABLE_WIFI)
  net::begin();   // Wi-Fi is a convenience mirror; cooking works without it
  ha::begin(net::mqtt_broker().c_str(), 1883, on_mute, on_target_abs, on_preset);
  net::set_settings_cbs(web_set_unit, on_mute, on_brightness, on_timezone);
  configTzTime(tz_posix(g_tz), "pool.ntp.org", "time.nist.gov");   // NTP clock
#endif

  hal::buzzer_play(hal::BuzzPattern::Chirp);
  Serial.println("[PanPilot] M4 ready — Target Assist + presets");
}

void loop() {
  static uint32_t last = 0;
  static uint32_t lastBatt = 0;
  static PowerFsm power;
  static PowerState prevPower = PowerState::ACTIVE;
  static BatteryMonitor battMon;
  lv_timer_handler();
  hal::buzzer_update();
  if (g_z2ReadyChirp) { g_z2ReadyChirp = false; hal::buzzer_play(hal::BuzzPattern::Double); }
#if defined(ENABLE_WIFI)
  net::loop();
  ha::loop();
  g_plug.setAlive(ha::plug_online()); // box birth/LWT status feeds S7 + arming
  g_plug.pump();                      // publish any staged duty (loop core owns MQTT)
  { struct tm ti;                     // NTP clock (getLocalTime returns fast)
    if (getLocalTime(&ti, 0)) {
      g_clockH = (uint8_t)ti.tm_hour; g_clockM = (uint8_t)ti.tm_min;
      g_timeValid = true;
    } }
#endif

  const uint32_t now = millis();

  static bool otaMarked = false;   // healthy runtime -> mark OTA image valid
  if (!otaMarked && now > 8000) { otaMarked = true; hal::ota_mark_stable(); }

  // Store a finished session here on the loop core (NVS + LittleFS writes),
  // then refresh the Last Cook screen. SensorTask only stages it.
  if (g_session_pending) {
    hal::session_store(g_pendingSession, g_trace, (uint16_t)g_traceN);
    g_session_pending = false;
    Serial.println("[session] saved to LittleFS");
    refresh_lastcook(ui::unit_useF());
  }
  if (g_persist_gains) { g_persist_gains = false;
    hal::storage_set_gains(g_kp, g_ki, g_kd); }

  // Poll the fuel gauge every 2 s (roadmap §2.1).
  if (now - lastBatt >= 2000) {
    lastBatt = now;
    g_batt = hal::power_read();
    BattEvent be = battMon.update(g_batt);
    if (be == BattEvent::LOW_BATT) Serial.println("[batt] low");
    else if (be == BattEvent::CRITICAL) {
      g_pluginWarn = true;                          // attention raises an L3 cue
      Serial.println("[batt] CRITICAL — PLUG ME IN");
    }
    if (g_batt.usbPresent) g_pluginWarn = false;    // cleared once plugged in
  }
  if (now - last >= 250) {          // 4 Hz UI refresh + serial dump
    last = now;
    Snapshot s;
    if (g_snap_mtx && xSemaphoreTake(g_snap_mtx, pdMS_TO_TICKS(5)) == pdTRUE) {
      s = g_snap;
      xSemaphoreGive(g_snap_mtx);
    }

    // Power/idle state machine (base spec §8).
    const uint32_t inactive = lv_disp_get_inactive_time(nullptr);
    g_lastTouchMs = (now >= inactive) ? now - inactive : 0;   // for interlock S10
    PowerState ps = power.step({inactive, s.sceneHot, s.heatingDetected});
    g_idle = (ps == PowerState::IDLE);
    if (ps != prevPower) {
      if (ps == PowerState::IDLE) {
        hal::display_set_brightness(BACKLIGHT_IDLE_BRIGHT);
        ui::show_idle();
      } else {
        // User's brightness level, capped on battery to save power (§2.1).
        hal::display_set_brightness(active_brightness());
        ui::show_home();
      }
      prevPower = ps;
    }
    if (power.wokeThisStep()) {
      hal::buzzer_play(hal::BuzzPattern::Chirp);
      if (power.wokeByHeat())
        Serial.println("[power] heating detected — select a target or preset");
    }

    // ---- Attention & cue escalation (roadmap §3.5) ----
    static AttentionManager attn;
    static ComplianceChecker comply;
    static bool prevTurnDown = false, complyFired = false;
    static bool strobing = false, strobeOn = false;
    static uint32_t strobeToggle = 0;
    if (s.has) {
      const UiState& u = s.ui;
      attn.setMuted(hal::buzzer_is_muted());
      if (u.pluginWarning)
        attn.raise(AttnLevel::L3, "PLUG ME IN", "battery critical", now);
      else if (u.guidance == GuidanceState::TOO_HOT)
        attn.raise(AttnLevel::L3, "TOO HOT", "", now);
      else if (u.recipeActive && u.recipeCue[0])
        attn.raise(AttnLevel::L2, u.recipeCue, "tap when done", now);
      else if (u.foodCue)
        attn.raise(AttnLevel::L2, u.foodCueVerb, u.foodCueSub, now);
      else if (u.addBatchPrompt)
        attn.raise(AttnLevel::L2, "ADD BATCH", "", now);
      else if (u.guidance == GuidanceState::TURN_DOWN_NOW)
        attn.raise(AttnLevel::L2, "TURN DOWN NOW", "", now);
      else if (u.guidance == GuidanceState::TURN_DOWN_SOON)
        attn.raise(AttnLevel::L2, "TURN DOWN SOON", "", now);
      else if (u.guidance == GuidanceState::READY)
        attn.raise(AttnLevel::L1, "READY", "", now);
      else
        attn.clear();

      // Compliance verification: after a TURN DOWN cue, confirm the rate fell.
      const bool turnDown = u.guidance == GuidanceState::TURN_DOWN_NOW;
      if (turnDown && !prevTurnDown) { comply.start(true, now); complyFired = false; }
      if (turnDown && !complyFired) {
        if (comply.update(u.rateCPerMin * 9.0f / 5.0f, now) == Compliance::COMPLIED) {
          hal::buzzer_play(hal::BuzzPattern::Chirp);   // confirm the knob turn
          complyFired = true;
          Serial.println("[attn] knob turn confirmed by rate change");
        }
      }
      if (!turnDown) comply.reset();
      prevTurnDown = turnDown;

      AttnOutput ao = attn.tick(now);
#if defined(ENABLE_WIFI)
      // Mirror the cue to Home Assistant the moment it changes (roadmap §3.5).
      static AttnLevel prevAttn = AttnLevel::L0;
      static const char* prevVerb = "";
      if (ao.level != prevAttn || ao.verb != prevVerb) {
        ha::publishAttention((int)ao.level, ao.verb, ao.sub);
        prevAttn = ao.level; prevVerb = ao.verb;
      }
#endif
      if (ao.buzz) {
        switch (ao.level) {
          case AttnLevel::L1: hal::buzzer_play(hal::BuzzPattern::Chirp); break;
          case AttnLevel::L2: hal::buzzer_play(hal::BuzzPattern::Double); break;
          case AttnLevel::L3: hal::buzzer_play(hal::BuzzPattern::Alarm); break;
          default: break;
        }
      }
      // Backlight strobe for L2/L3 (~2.5 Hz, under the 3 Hz cap), restore after.
      if (ao.strobe && !g_idle) {
        strobing = true;
        if (now - strobeToggle >= 200) {
          strobeToggle = now; strobeOn = !strobeOn;
          hal::display_set_brightness(strobeOn ? BACKLIGHT_ACTIVE_BRIGHT : 40);
        }
      } else if (strobing) {
        strobing = false;
        hal::display_set_brightness(active_brightness());
      }
    }

#if defined(ENABLE_WIFI)
    static uint32_t lastPub = 0;
    if (s.has && now - lastPub >= 500) {   // 2 Hz web push (roadmap §2.2)
      lastPub = now;
      net::publishState(s.ui, ui::unit_useF());
      net::publishThermal(s.frame);
    }
    static uint32_t lastHa = 0;
    if (s.has && now - lastHa >= 1000) {   // 1 Hz MQTT/HA state
      lastHa = now;
      ha::publish(s.ui, ui::unit_useF());
    }
#endif

    if (s.has && !g_idle) {
      ui::root_update(s.frame, s.reading, s.ui);
      const PanReading& r = s.reading;
      Serial.printf("[reading] %s pan=%.1fC disp=%.1fC rate=%.1fC/min conf=%u%s\n",
                    presence_str(r.presence), r.panTempC, s.ui.displayTempC,
                    s.ui.rateCPerMin, r.confidence,
                    r.stainlessHint ? " STAINLESS?" : "");
    }
  }
  delay(UI_TICK_MS);
}

#endif  // !PANPILOT_SIM
