// lv_conf.h — LVGL 8.3 configuration for PanPilot (base spec §9).
// Compact config: 16-bit color, custom tick, snapshot API on (screenshots).
#pragma once
#if 1  // enable content

#include <stdint.h>

// ---- Color ----
#define LV_COLOR_DEPTH        16
#ifdef PANPILOT_SIM
  #define LV_COLOR_16_SWAP    0     // native screenshot: straight RGB565
#else
  #define LV_COLOR_16_SWAP    1     // SPI ILI9488 wants byte-swapped RGB565
#endif

// ---- Memory ----
#define LV_MEM_CUSTOM         0
// LVGL heap. The classic ESP32 (basic board) has much tighter contiguous DRAM
// than the S3, and M1's frame-analysis scratch buffers eat into it — give it a
// smaller heap. The S3 gets 64 KB: the full screen set (created lazily, never
// destroyed) plus a 1800-pt Last Cook chart estimates at ~40 KB steady-state,
// which made 48 KB marginal. Boot logs lv_mem usage — bench-check it.
#ifdef BOARD_CROWPANEL35_BASIC
  #define LV_MEM_SIZE         (26U * 1024U)
#else
  #define LV_MEM_SIZE         (64U * 1024U)
#endif

// ---- HAL / tick ----
// We drive lv_tick_inc() from a millis()-based hook in ui_root (base spec §4).
#define LV_TICK_CUSTOM                1
#define LV_TICK_CUSTOM_INCLUDE        "lvgl_tick_hook.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR  (panpilot_millis())

#define LV_DPI_DEF            160

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
