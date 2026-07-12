// lv_conf.h — LVGL 8.3 configuration for PanPilot (base spec §9).
// Compact config: 16-bit color, custom tick, snapshot API on (screenshots).
#pragma once
#if 1  // enable content

#include <stdint.h>

// ---- Color ----
#define LV_COLOR_DEPTH        16
// NATIVE byte order everywhere. The flush callback hands LovyanGFX an
// lgfx::rgb565_t* (native RGB565) and lgfx does the panel conversion —
// exactly the factory lesson-03 pairing (their bundled lv_conf is SWAP=0).
// SWAP=1 here fed byte-swapped data declared as native: bench symptom was a
// pink background, yellow-instead-of-green buttons, fringed "fuzzy" text.
#define LV_COLOR_16_SWAP      0

// ---- Memory ----
#define LV_MEM_CUSTOM         0
// LVGL heap. The classic ESP32 (basic board) has much tighter contiguous DRAM
// than the S3, and M1's frame-analysis scratch buffers eat into it — give it a
// smaller heap. The S3 gets 64 KB: the full screen set (created lazily, never
// destroyed) plus a 1800-pt Last Cook chart estimates at ~40 KB steady-state,
// which made 48 KB marginal. Boot logs lv_mem usage — bench-check it.
#ifdef BOARD_CROWPANEL35_BASIC
  #define LV_MEM_SIZE         (26U * 1024U)
#elif defined(BOARD_HAS_PSRAM) && !defined(PANPILOT_SIM)
  // LVGL heap in PSRAM (bench 2026-07-12): at 64 KB internal the preset
  // EDITOR (textarea + keyboard, the largest screen) failed to create and
  // "+ New" did nothing — the food list had grown past 35 rows and screens
  // are never destroyed. Raising the static pool to 96 KB starved Wi-Fi
  // (~21 KB internal heap left), so the pool moves to the 8 MB PSRAM
  // entirely: objects/styles live there (S3 cache hides most latency), the
  // DMA draw buffers stay internal (static s_buf1/s_buf2 in display.cpp).
  #define LV_MEM_CUSTOM 1
  #define LV_MEM_CUSTOM_INCLUDE "esp_heap_caps.h"
  #define LV_MEM_CUSTOM_ALLOC(size) \
      heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
  #define LV_MEM_CUSTOM_REALLOC(p, size) \
      heap_caps_realloc(p, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
  #define LV_MEM_CUSTOM_FREE(p) heap_caps_free(p)
#else
  #define LV_MEM_SIZE         (64U * 1024U)
#endif

// ---- HAL / tick ----
// We drive lv_tick_inc() from a millis()-based hook in ui_root (base spec §4).
#define LV_TICK_CUSTOM                1
#define LV_TICK_CUSTOM_INCLUDE        "lvgl_tick_hook.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR  (panpilot_millis())

#define LV_DPI_DEF            160

// ---- Input / scrolling ----
// A press that moves this many px becomes a SCROLL and the release no longer
// clicks the row under the finger. LVGL's default 10 px let scroll flicks on
// the list screens land as taps ("changing settings by bonking a toggle" —
// bench 2026-07-11). 6 px still leaves comfortable slack for a stationary tap
// on the GT911 capacitive panel.
#define LV_INDEV_DEF_SCROLL_LIMIT  6

// ---- Features ----
#define LV_USE_SNAPSHOT      1     // kickoff: screenshot capture in the simulator
#define LV_USE_LOG           1
#if LV_USE_LOG
  #define LV_LOG_LEVEL       LV_LOG_LEVEL_WARN
  #define LV_LOG_PRINTF      1
#endif

#define LV_USE_PERF_MONITOR  0
#define LV_USE_MEM_MONITOR   0
#define LV_USE_ASSERT_NULL   1
#define LV_USE_ASSERT_MALLOC 1

// ---- Fonts (kitchen-distance huge numerals, base spec §9) ----
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_48 1
#define LV_FONT_DEFAULT &lv_font_montserrat_20

// ---- Widgets used through M0..M6 ----
#define LV_USE_LABEL   1
#define LV_USE_BTN     1
#define LV_USE_BAR     1
#define LV_USE_SLIDER  1
#define LV_USE_ARC     1
#define LV_USE_IMG     1
#define LV_USE_CANVAS  1     // thermal view (M1)
#define LV_USE_CHART   1     // last-cook sparkline (M11)
#define LV_USE_LIST    1     // food picker (M12.5)
#define LV_USE_SWITCH   1    // stainless toggle in the preset editor (Phase 2)
#define LV_USE_TEXTAREA 1    // preset-name entry (Phase 2)
#define LV_USE_BTNMATRIX 1   // backs the on-screen keyboard
#define LV_USE_KEYBOARD 1    // preset-name entry (Phase 2)

// ---- Theme ----
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1

#endif // enable content
