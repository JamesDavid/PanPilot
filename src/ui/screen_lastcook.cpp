// screen_lastcook.cpp — see screen_lastcook.h.
#include "screen_lastcook.h"

#include <cstdio>

#include "ui/ui_root.h"
#include "core/presets.h"

namespace ui {
namespace {
lv_obj_t* s_screen = nullptr;
lv_obj_t* s_chart = nullptr;
lv_chart_series_t* s_ser = nullptr;
lv_obj_t* s_title = nullptr;
lv_obj_t* s_stats = nullptr;
lv_obj_t* s_empty = nullptr;

void done_cb(lv_event_t*) { ui::show_home(); }
inline float cToF(float c) { return c * 9.0f / 5.0f + 32.0f; }
}  // namespace

lv_obj_t* lastcook_create() {
  if (s_screen) return s_screen;
  lv_obj_t* scr = lv_obj_create(nullptr);
  s_screen = scr;
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  s_title = lv_label_create(scr);
  lv_label_set_text(s_title, "Last Cook");
  lv_obj_set_style_text_font(s_title, &lv_font_montserrat_20, 0);
  lv_obj_align(s_title, LV_ALIGN_TOP_LEFT, 12, 8);

  s_chart = lv_chart_create(scr);
  lv_obj_set_size(s_chart, 456, 170);
  lv_obj_align(s_chart, LV_ALIGN_TOP_MID, 0, 40);
  lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
  lv_obj_set_style_bg_color(s_chart, lv_color_hex(0x0A0C0F), 0);
  lv_obj_set_style_size(s_chart, 0, LV_PART_INDICATOR);   // no point dots
  lv_chart_set_div_line_count(s_chart, 4, 0);
  s_ser = lv_chart_add_series(s_chart, lv_color_hex(0xE07000),
                              LV_CHART_AXIS_PRIMARY_Y);

  s_stats = lv_label_create(scr);
  lv_obj_set_style_text_font(s_stats, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_stats, lv_color_hex(0xB7C0CC), 0);
  lv_obj_align(s_stats, LV_ALIGN_BOTTOM_LEFT, 12, -14);

  s_empty = lv_label_create(scr);
  lv_label_set_text(s_empty, "No cooks logged yet");
  lv_obj_set_style_text_color(s_empty, lv_color_hex(0x8A93A0), 0);
  lv_obj_center(s_empty);

  lv_obj_t* done = lv_btn_create(scr);
  lv_obj_set_size(done, 84, 30);
  lv_obj_align(done, LV_ALIGN_TOP_RIGHT, -8, 6);
  lv_obj_add_event_cb(done, done_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* dl = lv_label_create(done);
  lv_label_set_text(dl, "Done");
  lv_obj_center(dl);
  return s_screen;
}

void lastcook_update(const SessionSummary& s, const int16_t* tr, int n,
                     bool hasData, bool useF) {
  if (!s_screen) lastcook_create();
  if (!hasData || n <= 0) {
    lv_obj_clear_flag(s_empty, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_chart, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(s_stats, "");
    return;
  }
  lv_obj_add_flag(s_empty, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s_chart, LV_OBJ_FLAG_HIDDEN);

  int16_t lo = tr[0], hi = tr[0];
  for (int i = 1; i < n; ++i) { if (tr[i] < lo) lo = tr[i]; if (tr[i] > hi) hi = tr[i]; }
  lv_chart_set_point_count(s_chart, n);
  lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, lo - 20, hi + 20);
  for (int i = 0; i < n; ++i) lv_chart_set_next_value(s_chart, s_ser, tr[i]);

  const float mx = useF ? cToF(s.maxTempC) : s.maxTempC;
  char buf[128];
  std::snprintf(buf, sizeof(buf),
                "%s  max %d\xC2\xB0%s  ready %us  overheat %us  food x%u",
                preset(s.presetId).name, (int)(mx + 0.5f), useF ? "F" : "C",
                (unsigned)s.timeInRangeSec, (unsigned)s.overheatSec,
                (unsigned)s.foodAddedCount);
  lv_label_set_text(s_stats, buf);

  char t[24];
  std::snprintf(t, sizeof(t), "Last Cook  (%d min)", n / 60);
  lv_label_set_text(s_title, t);
}

}  // namespace ui
