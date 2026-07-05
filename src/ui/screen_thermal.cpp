// screen_thermal.cpp — see screen_thermal.h. A 32x24 canvas painted with the
// ironbow palette and scaled up, with crosshair / ROI / label overlays drawn as
// child objects in scaled coordinates (keeps the canvas buffer tiny).
#include "screen_thermal.h"

#include <lvgl.h>
#include <cstdio>

#include "core/thermal_palette.h"

namespace ui {
namespace {

constexpr int SCALE = 12;                       // 32x24 -> 384x288
constexpr int IMG_W = THERM_COLS * SCALE;       // 384
constexpr int IMG_H = THERM_ROWS * SCALE;       // 288
constexpr int OX = (480 - IMG_W) / 2;           // image origin on screen
constexpr int OY = 8;

lv_obj_t* s_canvas = nullptr;
lv_color_t s_cbuf[THERM_COLS * THERM_ROWS];     // 32x24 RGB565 (1.5 KB)
lv_obj_t* s_roi = nullptr;
lv_obj_t* s_cross_h = nullptr;
lv_obj_t* s_cross_v = nullptr;
lv_obj_t* s_temp_lbl = nullptr;
lv_obj_t* s_range_lbl = nullptr;
lv_obj_t* s_hint_lbl = nullptr;

inline float cToF(float c) { return c * 9.0f / 5.0f + 32.0f; }

}  // namespace

void thermal_create() {
  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x0A0C0F), LV_PART_MAIN);

  s_canvas = lv_canvas_create(scr);
  lv_canvas_set_buffer(s_canvas, s_cbuf, THERM_COLS, THERM_ROWS,
                       LV_IMG_CF_TRUE_COLOR);
  lv_obj_set_pos(s_canvas, OX, OY);
  lv_img_set_antialias(s_canvas, false);
  lv_img_set_zoom(s_canvas, 256 * SCALE);        // nearest-neighbor upscale
  // zoom pivots at top-left so OX/OY stays the image origin
  lv_img_set_pivot(s_canvas, 0, 0);

  // Crosshair at frame center (aiming target, §9.2)
  auto mk_line = [&](int w, int h) {
    lv_obj_t* o = lv_obj_create(scr);
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_opa(o, LV_OPA_60, 0);
    lv_obj_set_style_bg_color(o, lv_color_hex(0xFFFFFF), 0);
    return o;
  };
  s_cross_h = mk_line(24, 2);
  s_cross_v = mk_line(2, 24);
  lv_obj_set_pos(s_cross_h, OX + IMG_W / 2 - 12, OY + IMG_H / 2 - 1);
  lv_obj_set_pos(s_cross_v, OX + IMG_W / 2 - 1, OY + IMG_H / 2 - 12);

  // ROI circle
  s_roi = lv_obj_create(scr);
  lv_obj_remove_style_all(s_roi);
  lv_obj_set_style_border_width(s_roi, 3, 0);
  lv_obj_set_style_border_color(s_roi, lv_color_hex(0x33FF88), 0);
  lv_obj_set_style_radius(s_roi, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(s_roi, LV_OPA_0, 0);

  // panTemp label
  s_temp_lbl = lv_label_create(scr);
  lv_obj_set_style_text_font(s_temp_lbl, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(s_temp_lbl, lv_color_hex(0xFFFFFF), 0);

  s_range_lbl = lv_label_create(scr);
  lv_obj_set_style_text_font(s_range_lbl, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_range_lbl, lv_color_hex(0x8A93A0), 0);
  lv_obj_align(s_range_lbl, LV_ALIGN_BOTTOM_LEFT, 8, -8);

  s_hint_lbl = lv_label_create(scr);
  lv_obj_set_style_text_font(s_hint_lbl, &lv_font_montserrat_20, 0);
  lv_obj_align(s_hint_lbl, LV_ALIGN_BOTTOM_MID, 0, -8);
}

void thermal_update(const ThermalFrame& f, const PanReading& r, bool useF) {
  if (!s_canvas) thermal_create();

  // Auto-scale palette to scene min/max (§9.2).
  float lo = f.px[0][0], hi = f.px[0][0];
  for (int y = 0; y < THERM_ROWS; ++y)
    for (int x = 0; x < THERM_COLS; ++x) {
      const float v = f.px[y][x];
      if (v < lo) lo = v; if (v > hi) hi = v;
    }
  const float span = (hi - lo) > 1.0f ? (hi - lo) : 1.0f;
  for (int y = 0; y < THERM_ROWS; ++y)
    for (int x = 0; x < THERM_COLS; ++x) {
      const RGB8 c = ironbow((f.px[y][x] - lo) / span);
      lv_canvas_set_px_color(s_canvas, x, y,
                             lv_color_make(c.r, c.g, c.b));
    }

  const bool present = r.presence == PanPresence::PRESENT ||
                       r.presence == PanPresence::UNCERTAIN;
  // ROI overlay follows the blob centroid (scaled to screen coords).
  if (present && r.roiPixelCount > 0) {
    const int dia = 64;  // fixed visual ROI marker (M1); blob-fit later
    lv_obj_set_size(s_roi, dia, dia);
    lv_obj_set_pos(s_roi, OX + int(r.roiCx * SCALE) - dia / 2,
                   OY + int(r.roiCy * SCALE) - dia / 2);
    lv_obj_clear_flag(s_roi, LV_OBJ_FLAG_HIDDEN);

    char buf[24];
    const float t = useF ? cToF(r.panTempC) : r.panTempC;
    std::snprintf(buf, sizeof(buf), "%d%s", int(t + 0.5f), useF ? "\xC2\xB0" "F" : "\xC2\xB0" "C");
    lv_label_set_text(s_temp_lbl, buf);
    lv_obj_set_pos(s_temp_lbl, OX + int(r.roiCx * SCALE) - 24,
                   OY + int(r.roiCy * SCALE) + dia / 2);
    lv_obj_clear_flag(s_temp_lbl, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(s_roi, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_temp_lbl, LV_OBJ_FLAG_HIDDEN);
  }

  char rng[40];
  const float loD = useF ? cToF(lo) : lo, hiD = useF ? cToF(hi) : hi;
  std::snprintf(rng, sizeof(rng), "range %d - %d%s", int(loD), int(hiD),
                useF ? "\xC2\xB0" "F" : "\xC2\xB0" "C");
  lv_label_set_text(s_range_lbl, rng);

  // Aim hint: is the blob near the crosshair? (§9.2)
  const float dcx = r.roiCx - THERM_COLS / 2.0f;
  const float dcy = r.roiCy - THERM_ROWS / 2.0f;
  const bool centered = present && (dcx * dcx + dcy * dcy) < 9.0f;
  if (r.presence == PanPresence::ABSENT) {
    lv_label_set_text(s_hint_lbl, "No pan in view");
    lv_obj_set_style_text_color(s_hint_lbl, lv_color_hex(0x8A93A0), 0);
  } else if (centered) {
    lv_label_set_text(s_hint_lbl, "Good aim");
    lv_obj_set_style_text_color(s_hint_lbl, lv_color_hex(0x33FF88), 0);
  } else {
    lv_label_set_text(s_hint_lbl, "Center the pan in view");
    lv_obj_set_style_text_color(s_hint_lbl, lv_color_hex(0xFFC000), 0);
  }
}

}  // namespace ui
