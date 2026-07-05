// main.cpp — PanPilot entry point (base spec §3: setup + task spawn only).
// M2: Thermometer Mode. SensorTask (core 0) reads the MLX90640, runs
// frame_analysis + thermal_model, and publishes a UiState snapshot; the UI loop
// (core 1) renders the home screen (temp/rate/trend) with the thermal view one
// tap away, and dumps readings over serial.
#if !defined(PANPILOT_SIM)

#include <Arduino.h>
#include <lvgl.h>
#include <algorithm>
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
#include "hal/display.h"
#include "hal/buzzer.h"
#include "hal/storage.h"
#include "hal/i2c_bus.h"
#include "sensor/mlx90640_source.h"
#include "sensor/frame_analysis.h"
#include "ui/ui_root.h"

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

// Target owned by the logic layer; UI sends adjust/select commands (base spec
// §7.3). Guarded because SensorTask (core 0) reads it while UI callbacks (loop,
// core 1) write it.
Target g_target;
volatile uint8_t g_presetId = PRESET_GENERIC;
SemaphoreHandle_t g_target_mtx = nullptr;

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

void on_learn(uint8_t cmd) {
  if (cmd == 1) {                       // primary: Start (off) or Save (done)
    if (g_learn_phase == 0) {
      g_learn_peak = 0; g_learn_progress = 0;
      g_learn_start = millis(); g_learn_phase = 1;
    } else if (g_learn_phase == 2) {
      PanProfile p = make_profile("Pan", g_learn_peak);
      hal::storage_save_profile(p);
      g_applied_lag = p.lagMinutes;
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

void fire_alert(AlertAction a) {
  switch (a) {
    case AlertAction::READY_CHIME:   hal::buzzer_play(hal::BuzzPattern::Long);   break;
    case AlertAction::TURN_DOWN:     hal::buzzer_play(hal::BuzzPattern::Double); break;
    case AlertAction::TOO_HOT_ALARM: hal::buzzer_play(hal::BuzzPattern::Alarm);  break;
    default: break;
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
  for (;;) {
    if (!g_sensor_ok) {
      g_sensor_ok = sensor::mlx_begin();
      if (!g_sensor_ok) { vTaskDelay(pdMS_TO_TICKS(2000)); continue; }
      Serial.println("[PanPilot] MLX90640 online");
    }
    if (sensor::mlx_read(frame)) {
      PanReading r = analyzer.process(frame);
      model.update(r);
      uint8_t presetId = PRESET_GENERIC;
      if (xSemaphoreTake(g_target_mtx, pdMS_TO_TICKS(10)) == pdTRUE) {
        target = g_target; presetId = g_presetId;
        xSemaphoreGive(g_target_mtx);
      }

      GuidanceInput gi;
      gi.tempF = ThermalModel::cToF(model.displayTempC());
      gi.rateFPerMin = model.rateFPerMin();
      gi.confidence = r.confidence;
      gi.presence = r.presence;
      gi.moved = r.moved;
      gi.lagMinutes = g_applied_lag;    // learned lag (Learn Pan Mode)
      GuidanceOutput go = guidance.step(gi, target, millis());
      fire_alert(go.alert);

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
        Serial.println("[recovery] food added");
      } else if (ro.event == RecoveryEvent::RECOVERED) {
        hal::buzzer_play(hal::BuzzPattern::Long);
        addBatchUntil = nowm + 4000;
        Serial.println("[recovery] pan recovered — add next batch");
      }
      const bool addBatch = nowm < addBatchUntil;

      // Session lifecycle (§8): start on a hot pan, end when cool + stable.
      if (!session.active() && r.presence == PanPresence::PRESENT && sceneHot)
        session.begin(presetId, nowm);
      if (session.active()) {
        session.update(r, go.state, nowm);
        if (r.panTempC < frame.ambientC + SESSION_END_AMBIENT_C) {
          if (coolStart == 0) coolStart = nowm;
          else if (nowm - coolStart > SESSION_END_STABLE_MS) {
            hal::storage_session_push(session.finish(nowm));
            coolStart = 0;
            Serial.println("[session] saved");
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
      u.guidance = ro.recovering ? GuidanceState::RECOVERING : go.state;
      u.targetCenterF = target.centerF;
      u.presetId = presetId;
      u.etaSeconds = go.etaSeconds;
      u.projectedPeakF = go.projectedPeakF;
      u.recovering = ro.recovering;
      u.recoveryHint = ro.hint;
      u.addBatchPrompt = addBatch;
      u.learnPhase = (LearnPhase)g_learn_phase;
      u.learnProgress = g_learn_progress;
      u.learnedLagMinutes = g_learned_lag;

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

  hal::i2c_bus_init();
  hal::storage_begin();
  hal::display_begin();
  hal::buzzer_begin();
  hal::buzzer_set_muted(hal::storage_get_muted());

  g_target_mtx = xSemaphoreCreateMutex();
  { int lo, hi, warn, pid;
    hal::storage_get_target(lo, hi, warn, pid);
    g_target.loF = lo; g_target.hiF = hi; g_target.warnF = warn;
    g_target.centerF = (lo + hi) / 2; g_presetId = (uint8_t)pid; }
  { PanProfile pf;
    if (hal::storage_load_profile(pf)) { g_applied_lag = pf.lagMinutes;
      Serial.printf("[profile] loaded lag=%.2f min\n", pf.lagMinutes); } }
  ui::root_init(hal::storage_get_unit_useF(), persist_unit, on_target_delta,
                on_preset, on_learn);

  g_snap_mtx = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(SensorTask, "sensor", 8192, nullptr, 2, nullptr, 0);

  hal::buzzer_play(hal::BuzzPattern::Chirp);
  Serial.println("[PanPilot] M4 ready — Target Assist + presets");
}

void loop() {
  static uint32_t last = 0;
  static PowerFsm power;
  static PowerState prevPower = PowerState::ACTIVE;
  lv_timer_handler();
  hal::buzzer_update();

  const uint32_t now = millis();
  if (now - last >= 250) {          // 4 Hz UI refresh + serial dump
    last = now;
    Snapshot s;
    if (g_snap_mtx && xSemaphoreTake(g_snap_mtx, pdMS_TO_TICKS(5)) == pdTRUE) {
      s = g_snap;
      xSemaphoreGive(g_snap_mtx);
    }

    // Power/idle state machine (base spec §8).
    const uint32_t inactive = lv_disp_get_inactive_time(nullptr);
    PowerState ps = power.step({inactive, s.sceneHot, s.heatingDetected});
    g_idle = (ps == PowerState::IDLE);
    if (ps != prevPower) {
      if (ps == PowerState::IDLE) {
        hal::display_set_brightness(BACKLIGHT_IDLE_BRIGHT);
        ui::show_idle();
      } else {
        hal::display_set_brightness(BACKLIGHT_ACTIVE_BRIGHT);
        ui::show_home();
      }
      prevPower = ps;
    }
    if (power.wokeThisStep()) {
      hal::buzzer_play(hal::BuzzPattern::Chirp);
      if (power.wokeByHeat())
        Serial.println("[power] heating detected — select a target or preset");
    }

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
