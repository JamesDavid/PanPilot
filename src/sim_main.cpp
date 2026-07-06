// sim_main.cpp — headless LVGL renderer for screenshots (kickoff §Screenshots).
// Builds natively (env:sim, host gcc), compiles the REAL ui/ code against a
// full-screen memory framebuffer, renders a screen/state, and writes a PPM
// (scripts/screenshots.py converts to PNG). No SDL — offscreen only.
//
// Device HAL TUs (display/buzzer/main) compile to nothing under PANPILOT_SIM;
// this file provides the buzzer stub ui_root links against, plus the LVGL tick.
#if defined(PANPILOT_SIM)

#include <lvgl.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "hal/buzzer.h"
#include "ui/screen_home.h"
#include "ui/screen_thermal.h"
#include "ui/screen_presets.h"
#include "ui/screen_learn.h"
#include "ui/screen_lastcook.h"
#include "ui/screen_foods.h"
#include "ui/screen_settings.h"
#include "ui/screen_preset_edit.h"
#include "ui/screen_assist.h"
#include "ui/screen_onboarding.h"
#include "ui/screen_autotune.h"
#include "ui/screen_profiles.h"
#include "core/profilestore.h"
#include "core/foodlib.h"
#include "core/app_state.h"
#include "core/thermal_model.h"
#include "sensor/frame_analysis.h"
#include <cmath>
#include <string>

// ---- buzzer stub (ui_root calls buzzer_play on button click; silent here) ----
namespace hal {
void buzzer_begin() {}
void buzzer_play(BuzzPattern) {}
void buzzer_update() {}
void buzzer_set_muted(bool) {}
bool buzzer_is_muted() { return false; }
}  // namespace hal

// ---- LVGL custom tick source (lv_conf.h -> panpilot_millis()) ----
extern "C" uint32_t panpilot_millis(void) {
  static const auto t0 = std::chrono::steady_clock::now();
  return static_cast<uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - t0)
          .count());
}

namespace {
constexpr int kW = 480;
constexpr int kH = 320;
lv_color_t g_fb[kW * kH];

void flush_cb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* px) {
  for (int y = area->y1; y <= area->y2; ++y)
    for (int x = area->x1; x <= area->x2; ++x) g_fb[y * kW + x] = *px++;
  lv_disp_flush_ready(drv);
}

void write_ppm(const char* path) {
  FILE* f = std::fopen(path, "wb");
  if (!f) { std::fprintf(stderr, "cannot open %s\n", path); std::exit(2); }
  std::fprintf(f, "P6\n%d %d\n255\n", kW, kH);
  for (int i = 0; i < kW * kH; ++i) {
    const uint16_t c = g_fb[i].full;              // RGB565 (LV_COLOR_16_SWAP=0)
    const uint8_t r5 = (c >> 11) & 0x1F, g6 = (c >> 5) & 0x3F, b5 = c & 0x1F;
    const uint8_t rgb[3] = {
        static_cast<uint8_t>((r5 << 3) | (r5 >> 2)),
        static_cast<uint8_t>((g6 << 2) | (g6 >> 4)),
        static_cast<uint8_t>((b5 << 3) | (b5 >> 2))};
    std::fwrite(rgb, 1, 3, f);
  }
  std::fclose(f);
}
}  // namespace

// Synthetic hot-disc frame (a pan) for the thermal scene — mirrors the test
// generator so the rendered view matches what frame_analysis sees.
ThermalFrame synth_pan() {
  ThermalFrame f{};
  f.ambientC = 26.0f; f.t_ms = 1000; f.valid = true;
  for (int r = 0; r < THERM_ROWS; ++r)
    for (int c = 0; c < THERM_COLS; ++c) {
      const float d = std::hypot(c - 17.0f, r - 12.0f);
      f.px[r][c] = d <= 6.0f ? 205.0f - d * 3.0f      // hot pan, cooler rim
                             : 26.0f + (d < 10 ? (10 - d) : 0);
    }
  return f;
}

int main(int argc, char** argv) {
  const char* out = (argc > 1) ? argv[1] : "screenshot.ppm";
  const std::string scene = (argc > 2) ? argv[2] : "home";

  lv_init();
  static lv_disp_draw_buf_t draw_buf;
  static lv_color_t b1[kW * 48], b2[kW * 48];
  lv_disp_draw_buf_init(&draw_buf, b1, b2, kW * 48);

  static lv_disp_drv_t drv;
  lv_disp_drv_init(&drv);
  drv.hor_res = kW;
  drv.ver_res = kH;
  drv.flush_cb = flush_cb;
  drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&drv);

  auto fToC = [](float f) { return (f - 32.0f) * 5.0f / 9.0f; };
  if (scene == "thermal") {
    ThermalFrame f = synth_pan();
    FrameAnalyzer fa;
    PanReading r = fa.process(f);
    lv_scr_load(ui::thermal_create());
    ui::thermal_update(f, r, /*useF=*/true);
  } else if (scene == "presets") {
    lv_scr_load(ui::presets_create());
  } else if (scene == "foods") {
    lv_scr_load(ui::foods_create());
  } else if (scene == "settings") {
    lv_scr_load(ui::settings_create());
    ui::settings_update(/*useF=*/true, /*muted=*/false, /*brightness=*/2);
  } else if (scene == "presetedit") {
    lv_scr_load(ui::preset_edit_create());
    ui::preset_edit_load("Smash burger", 450, 500, false, /*canDelete=*/false);
  } else if (scene == "profiles") {
    static ProfileStore ps;
    ps.add(make_profile("Cast iron 10\"", 42));
    ps.add(make_profile("Nonstick", 16));
    ps.add(make_profile("Carbon steel", 55));
    ps.setActive(0);
    lv_scr_load(ui::profiles_create());
    ui::profiles_update(ps);
  } else if (scene == "onboarding") {
    lv_scr_load(ui::onboarding_create());
    ui::onboarding_reset(/*useF=*/true);
  } else if (scene == "autotune") {
    UiState u;
    u.assistArmed = true;
    u.autotuneState = 2;               // show the result state
    u.autotuneProgress = 5;
    u.autotuneKp = 0.382f;
    u.autotuneKi = 0.109f;
    u.autotuneKd = 0.334f;
    lv_scr_load(ui::autotune_create());
    ui::autotune_update(u);
  } else if (scene == "assistarm") {
    lv_scr_load(ui::assist_create());
    ui::assist_load("SSR box", /*ready=*/true);
  } else if (scene == "assist") {
    UiState u;
    u.mode = Mode::TARGET;
    u.presence = PanPresence::PRESENT;
    u.modelValid = true;
    u.confidence = 90;
    u.displayTempC = fToC(455);
    u.rateCPerMin = 2;
    u.trend = Trend::RISING_SLOW;
    u.guidance = GuidanceState::HOLD;
    u.targetCenterF = 475;
    u.presetId = PRESET_SEAR;
    u.assistArmed = true;
    u.actuatorAvailable = true;
    u.assistDuty = 0.45f;
    u.actuatorName = "SSR box";
    lv_scr_load(ui::home_create());
    ui::home_update(u, true);
  } else if (scene == "cooking") {
    UiState u;
    u.mode = Mode::TARGET;
    u.presence = PanPresence::PRESENT;
    u.modelValid = true;
    u.confidence = 92;
    u.displayTempC = fToC(365);
    u.rateCPerMin = 1.0f;
    u.trend = Trend::STABLE;
    u.guidance = GuidanceState::READY;
    u.targetCenterF = 362;
    u.food = &foodlib_entry(0);        // Pancakes
    u.foodTimer.phase = FoodTimerOut::COOKING;
    u.foodTimer.side = 1;
    u.foodTimer.remainingSec = 62;
    u.foodTimer.k = 1.0f;
    u.batchCount = 1;
    lv_scr_load(ui::home_create());
    ui::home_update(u, true);
  } else if (scene == "feedback") {
    UiState u;
    u.mode = Mode::TARGET;
    u.presence = PanPresence::PRESENT;
    u.modelValid = true;
    u.confidence = 90;
    u.displayTempC = fToC(360);
    u.guidance = GuidanceState::READY;
    u.food = &foodlib_entry(0);        // Pancakes just removed
    u.feedbackPrompt = true;
    u.feedbackName = foodlib_entry(0).name;
    lv_scr_load(ui::home_create());
    ui::home_update(u, true);
  } else if (scene == "multipan") {
    UiState u;
    u.mode = Mode::TARGET;
    u.presence = PanPresence::PRESENT;
    u.modelValid = true;
    u.confidence = 88;
    u.displayTempC = fToC(470);        // primary: Sear, still heating
    u.rateCPerMin = 12;
    u.trend = Trend::RISING_SLOW;
    u.guidance = GuidanceState::HOLD;
    u.targetCenterF = 512;
    u.presetId = PRESET_SEAR;
    u.zone2Present = true;             // secondary: Eggs at 300
    u.zone2TempC = fToC(300);
    u.zone2Guidance = GuidanceState::READY;
    u.zone2TargetF = 300;
    lv_scr_load(ui::home_create());
    ui::home_update(u, true);
  } else if (scene == "learn") {
    UiState u;
    u.learnPhase = LearnPhase::DONE;
    u.learnedLagMinutes = 0.7f;
    lv_scr_load(ui::learn_create());
    ui::learn_update(u);
  } else if (scene == "lastcook") {
    static int16_t tr[600];
    for (int i = 0; i < 600; ++i) {           // preheat ramp -> plateau w/ dips
      float c = 25 + 150 * (1 - std::exp(-i / 90.0f));
      if (i > 300 && (i % 120) < 8) c -= 25;  // food-added dips
      tr[i] = (int16_t)(c * 10);
    }
    SessionSummary s{};
    s.presetId = 1; s.maxTempC = 178; s.timeInRangeSec = 420;
    s.overheatSec = 12; s.foodAddedCount = 3;
    lv_scr_load(ui::lastcook_create());
    ui::lastcook_update(s, tr, 600, true, true);
  } else {  // "home" (Target Assist, HOLD) or "ready" (full-screen alert)
    UiState u;
    u.mode = Mode::TARGET;
    u.presence = PanPresence::PRESENT;
    u.modelValid = true;
    u.confidence = 91;
    u.targetCenterF = 350;
    u.rateCPerMin = 8.0f;          // ~14 F/min
    u.trend = Trend::RISING_SLOW;
    if (scene == "ready") {
      u.displayTempC = fToC(350);
      u.rateCPerMin = 1.0f; u.trend = Trend::STABLE;
      u.guidance = GuidanceState::READY;
      u.etaSeconds = -1;
    } else {
      u.displayTempC = fToC(300);  // heating toward 350
      u.guidance = GuidanceState::HOLD;
      u.etaSeconds = 135;
    }
    lv_scr_load(ui::home_create());
    ui::home_update(u, /*useF=*/true);
  }

  // Pump LVGL until fully drawn (static screen settles in a few passes).
  for (int i = 0; i < 40; ++i) lv_timer_handler();

  write_ppm(out);
  std::printf("wrote %s (%dx%d)\n", out, kW, kH);
  return 0;
}

#endif  // PANPILOT_SIM
