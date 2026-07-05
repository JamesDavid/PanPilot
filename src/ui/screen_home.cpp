// screen_home.cpp — see screen_home.h.
#include "screen_home.h"

#include <cstdio>

#include "ui/ui_root.h"

namespace ui {
namespace {

lv_obj_t* s_screen = nullptr;
lv_obj_t* s_mode = nullptr;
lv_obj_t* s_conf = nullptr;
lv_obj_t* s_unit_btn_lbl = nullptr;
lv_obj_t* s_temp = nullptr;
lv_obj_t* s_rate = nullptr;
lv_obj_t* s_state = nullptr;
lv_obj_t* s_note = nullptr;

void temp_tap_cb(lv_event_t*) { ui::show_thermal(); }
void unit_cb(lv_event_t*) { ui::toggle_unit(); }

inline float cToF(float c) { return c * 9.0f / 5.0f + 32.0f; }

}  // namespace

lv_obj_t* home_create() {
  if (s_screen) return s_screen;
  lv_obj_t* scr = lv_obj_create(nullptr);
  s_screen = scr;
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  // --- status bar ---
  s_mode = lv_label_create(scr);
  lv_label_set_text(s_mode, "THERMOMETER");
  lv_obj_set_style_text_color(s_mode, lv_color_hex(0x8A93A0), 0);
  lv_obj_set_style_text_font(s_mode, &lv_font_montserrat_14, 0);
  lv_obj_align(s_mode, LV_ALIGN_TOP_LEFT, 10, 8);

  s_conf = lv_label_create(scr);
  lv_obj_set_style_text_color(s_conf, lv_color_hex(0x8A93A0), 0);
  lv_obj_set_style_text_font(s_conf, &lv_font_montserrat_14, 0);
  lv_obj_align(s_conf, LV_ALIGN_TOP_RIGHT, -84, 8);

  lv_obj_t* ubtn = lv_btn_create(scr);
  lv_obj_set_size(ubtn, 64, 34);
  lv_obj_align(ubtn, LV_ALIGN_TOP_RIGHT, -8, 4);
  lv_obj_set_style_bg_color(ubtn, lv_color_hex(0x2A323C), 0);
  lv_obj_add_event_cb(ubtn, unit_cb, LV_EVENT_CLICKED, nullptr);
  s_unit_btn_lbl = lv_label_create(ubtn);
  lv_label_set_text(s_unit_btn_lbl, "\xC2\xB0" "F");
  lv_obj_center(s_unit_btn_lbl);

  // --- big temperature (tap -> thermal view) ---
  s_temp = lv_label_create(scr);
  lv_label_set_text(s_temp, "--");
  lv_obj_set_style_text_font(s_temp, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(s_temp, lv_color_hex(0xF5F5F5), 0);
  lv_obj_align(s_temp, LV_ALIGN_CENTER, 0, -28);
  lv_obj_add_flag(s_temp, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_ext_click_area(s_temp, 40);
  lv_obj_add_event_cb(s_temp, temp_tap_cb, LV_EVENT_CLICKED, nullptr);

  s_rate = lv_label_create(scr);
  lv_obj_set_style_text_font(s_rate, &lv_font_montserrat_28, 0);
  lv_obj_align(s_rate, LV_ALIGN_CENTER, 0, 30);

  s_state = lv_label_create(scr);
  lv_obj_set_style_text_font(s_state, &lv_font_montserrat_20, 0);
  lv_obj_align(s_state, LV_ALIGN_CENTER, 0, 74);

  s_note = lv_label_create(scr);
  lv_obj_set_style_text_font(s_note, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_note, lv_color_hex(0xFFC000), 0);
  lv_obj_align(s_note, LV_ALIGN_BOTTOM_MID, 0, -8);
  return s_screen;
}

void home_update(const UiState& s, bool useF) {
  if (!s_screen) home_create();
  char buf[40];

  lv_label_set_text(s_unit_btn_lbl, useF ? "\xC2\xB0" "F" : "\xC2\xB0" "C");
  std::snprintf(buf, sizeof(buf), "conf %u%%", s.confidence);
  lv_label_set_text(s_conf, buf);

  const bool absent = s.presence == PanPresence::ABSENT;
  if (absent || !s.modelValid) {
    lv_label_set_text(s_temp, "--");
    lv_label_set_text(s_rate, "");
    lv_label_set_text(s_state, "No pan - aim the sensor");
    lv_obj_set_style_text_color(s_state, lv_color_hex(0x8A93A0), 0);
    lv_label_set_text(s_note, "");
    return;
  }

  const float t = useF ? cToF(s.displayTempC) : s.displayTempC;
  std::snprintf(buf, sizeof(buf), "%d\xC2\xB0%s", int(t + 0.5f), useF ? "F" : "C");
  lv_label_set_text(s_temp, buf);

  // rate + trend arrow
  const float rate = useF ? s.rateCPerMin * 9.0f / 5.0f : s.rateCPerMin;
  const char* arrow = LV_SYMBOL_MINUS;
  uint32_t col = 0x6EA8FE;
  switch (s.trend) {
    case Trend::RISING_SLOW: arrow = LV_SYMBOL_UP;   col = 0xFFC000; break;
    case Trend::RISING_FAST: arrow = LV_SYMBOL_UP;   col = 0xFF7043; break;
    case Trend::COOLING:     arrow = LV_SYMBOL_DOWN; col = 0x6EA8FE; break;
    default:                 arrow = LV_SYMBOL_MINUS; col = 0x8A93A0; break;
  }
  if (s.rateCPerMin == 0.0f && s.trend == Trend::STABLE)
    std::snprintf(buf, sizeof(buf), "estimating...");
  else
    std::snprintf(buf, sizeof(buf), "%s %d\xC2\xB0%s/min", arrow,
                  int(rate + (rate < 0 ? -0.5f : 0.5f)), useF ? "F" : "C");
  lv_label_set_text(s_rate, buf);
  lv_obj_set_style_text_color(s_rate, lv_color_hex(col), 0);

  if (s.moved) {
    lv_label_set_text(s_state, "Check aim");
    lv_obj_set_style_text_color(s_state, lv_color_hex(0xFFC000), 0);
  } else {
    const char* txt = "Stable";
    if (s.trend == Trend::RISING_FAST) txt = "Heating fast";
    else if (s.trend == Trend::RISING_SLOW) txt = "Heating";
    else if (s.trend == Trend::COOLING) txt = "Cooling";
    lv_label_set_text(s_state, txt);
    lv_obj_set_style_text_color(s_state, lv_color_hex(0xF5F5F5), 0);
  }
  lv_label_set_text(s_note, s.stainlessHint
                                ? "Bare stainless reads low - trust the trend"
                                : "");
}

}  // namespace ui
