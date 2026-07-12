// screen_burnermap.cpp — see screen_burnermap.h.
#include "screen_burnermap.h"

#include <cstdio>

#include "ui/ui_root.h"
#include "core/burnermap.h"

namespace ui {
namespace {
lv_obj_t* s_screen = nullptr;
lv_obj_t* s_title = nullptr;
lv_obj_t* s_body = nullptr;
lv_obj_t* s_count = nullptr;      // big countdown during measure windows
lv_obj_t* s_results = nullptr;    // DONE: predicted hold temps
lv_obj_t* s_primary = nullptr;
lv_obj_t* s_primary_lbl = nullptr;
lv_obj_t* s_secondary_lbl = nullptr;
uint8_t s_phase = 0;
bool s_canSave = false;

void primary_cb(lv_event_t*) {
  // IDLE: start. AWAIT_*: knob confirmed. DONE: save.
  if (s_phase == 0) ui::burnermap_cmd(0);
  else if (s_phase == 1 || s_phase == 4) ui::burnermap_cmd(1);
  else if (s_phase == 7 && s_canSave) ui::burnermap_cmd(3);
}
void secondary_cb(lv_event_t*) { ui::burnermap_cmd(2); }   // cancel / back
}  // namespace

lv_obj_t* burnermap_create() {
  if (s_screen) return s_screen;
  lv_obj_t* scr = lv_obj_create(nullptr);
  s_screen = scr;
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  s_title = lv_label_create(scr);
  lv_obj_set_style_text_font(s_title, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(s_title, lv_color_hex(0xF5F5F5), 0);
  lv_obj_align(s_title, LV_ALIGN_TOP_MID, 0, 22);

  s_body = lv_label_create(scr);
  lv_label_set_long_mode(s_body, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(s_body, 440);
  lv_obj_set_style_text_align(s_body, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(s_body, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_body, lv_color_hex(0xB7BEC8), 0);
  lv_obj_align(s_body, LV_ALIGN_TOP_MID, 0, 66);

  s_count = lv_label_create(scr);
  lv_obj_set_style_text_font(s_count, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(s_count, lv_color_hex(0xE07000), 0);
  lv_obj_align(s_count, LV_ALIGN_CENTER, 0, 10);
  lv_obj_add_flag(s_count, LV_OBJ_FLAG_HIDDEN);

  s_results = lv_label_create(scr);
  lv_obj_set_style_text_font(s_results, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_results, lv_color_hex(0x5AA0FF), 0);
  lv_obj_set_style_text_align(s_results, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(s_results, LV_ALIGN_CENTER, 0, 20);
  lv_obj_add_flag(s_results, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t* sec = lv_btn_create(scr);
  lv_obj_set_size(sec, 150, 44);
  lv_obj_align(sec, LV_ALIGN_BOTTOM_LEFT, 12, -10);
  lv_obj_set_style_bg_color(sec, lv_color_hex(0x2A323C), 0);
  lv_obj_add_event_cb(sec, secondary_cb, LV_EVENT_CLICKED, nullptr);
  s_secondary_lbl = lv_label_create(sec);
  lv_obj_set_style_text_font(s_secondary_lbl, &lv_font_montserrat_20, 0);
  lv_obj_center(s_secondary_lbl);

  s_primary = lv_btn_create(scr);
  lv_obj_set_size(s_primary, 190, 44);
  lv_obj_align(s_primary, LV_ALIGN_BOTTOM_RIGHT, -12, -10);
  lv_obj_set_style_bg_color(s_primary, lv_color_hex(0x2E7D32), 0);
  lv_obj_add_event_cb(s_primary, primary_cb, LV_EVENT_CLICKED, nullptr);
  s_primary_lbl = lv_label_create(s_primary);
  lv_obj_set_style_text_font(s_primary_lbl, &lv_font_montserrat_20, 0);
  lv_obj_center(s_primary_lbl);
  return s_screen;
}

void burnermap_update(const UiState& s, bool useF) {
  if (!s_screen) burnermap_create();
  s_phase = s.bmapPhase;
  s_canSave = s.bmapCanSave;
  char b[200];

  lv_obj_add_flag(s_count, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_results, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_state(s_primary, LV_STATE_DISABLED);

  switch (s.bmapPhase) {
    case 1:   // AWAIT_KNOB
      lv_label_set_text(s_title, "Map Burner");
      std::snprintf(b, sizeof(b),
                    "Step %d of %d\nSet the burner knob to %s, then tap Ready.",
                    s.bmapSetting + 1, BURNER_SETTINGS,
                    burner_setting_name(s.bmapSetting));
      lv_label_set_text(s_body, b);
      lv_label_set_text(s_primary_lbl, "Ready");
      lv_label_set_text(s_secondary_lbl, "Cancel");
      break;
    case 2:   // SETTLING
    case 3: { // MEASURING
      lv_label_set_text(s_title, "Map Burner");
      std::snprintf(b, sizeof(b), "%s %s - keep the knob still",
                    s.bmapPhase == 2 ? "Settling at" : "Measuring",
                    burner_setting_name(s.bmapSetting));
      lv_label_set_text(s_body, b);
      std::snprintf(b, sizeof(b), "%us", (unsigned)s.bmapSecsLeft);
      lv_label_set_text(s_count, b);
      lv_obj_clear_flag(s_count, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(s_primary_lbl, "...");
      lv_obj_add_state(s_primary, LV_STATE_DISABLED);
      lv_label_set_text(s_secondary_lbl, "Cancel");
      break;
    }
    case 4:   // AWAIT_OFF
      lv_label_set_text(s_title, "Map Burner");
      lv_label_set_text(s_body,
          "Last step: turn the burner OFF, then tap Ready.\n"
          "PanPilot measures how fast this pan sheds heat.");
      lv_label_set_text(s_primary_lbl, "Ready");
      lv_label_set_text(s_secondary_lbl, "Cancel");
      break;
    case 5:   // COOL_SETTLING
    case 6:   // COOL_MEASURING
      lv_label_set_text(s_title, "Map Burner");
      lv_label_set_text(s_body, "Measuring cool-down - leave the pan alone");
      std::snprintf(b, sizeof(b), "%us", (unsigned)s.bmapSecsLeft);
      lv_label_set_text(s_count, b);
      lv_obj_clear_flag(s_count, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(s_primary_lbl, "...");
      lv_obj_add_state(s_primary, LV_STATE_DISABLED);
      lv_label_set_text(s_secondary_lbl, "Cancel");
      break;
    case 7: { // DONE
      lv_label_set_text(s_title, "Burner mapped!");
      lv_label_set_text(s_body,
          s.bmapCanSave ? "Predicted hold temperature per knob setting:"
                        : "SIMULATED data (dev build) - cannot be saved:");
      int n = 0;
      b[0] = '\0';
      for (int i = 0; i < BURNER_SETTINGS; ++i) {
        int t = s.bmapSettleF[i];
        if (!useF) t = (t - 32) * 5 / 9;
        n += std::snprintf(b + n, sizeof(b) - n, "%s%s  ~%d\xC2\xB0%s",
                           i ? "\n" : "", burner_setting_name(i), t,
                           useF ? "F" : "C");
      }
      lv_label_set_text(s_results, b);
      lv_obj_clear_flag(s_results, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(s_primary_lbl, "Save to pan");
      if (!s.bmapCanSave) lv_obj_add_state(s_primary, LV_STATE_DISABLED);
      lv_label_set_text(s_secondary_lbl, "Discard");
      break;
    }
    default:  // IDLE
      lv_label_set_text(s_title, "Map Burner");
      lv_label_set_text(s_body,
          "Calibrates the ACTIVE pan on this burner (~5 min): you'll set the "
          "knob to each position while PanPilot measures the heating rate, "
          "then turn it off for a cool-down. Down-cues will then suggest knob "
          "settings from THIS pan's data. Start with the pan warm-ish but not "
          "screaming hot.");
      lv_label_set_text(s_primary_lbl, LV_SYMBOL_PLAY "  Start");
      lv_label_set_text(s_secondary_lbl, "Back");
      break;
  }
}

}  // namespace ui
