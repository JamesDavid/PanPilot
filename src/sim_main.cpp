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
#include "ui/ui_root.h"

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

int main(int argc, char** argv) {
  const char* out = (argc > 1) ? argv[1] : "screenshot.ppm";
  // (argv[2] will select the scene/state as more screens land in M1+.)

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

  ui::root_init();

  // Pump LVGL until fully drawn (static screen settles in a few passes).
  for (int i = 0; i < 40; ++i) lv_timer_handler();

  write_ppm(out);
  std::printf("wrote %s (%dx%d)\n", out, kW, kH);
  return 0;
}

#endif  // PANPILOT_SIM
