// screen_learn.cpp — see screen_learn.h.
#include "screen_learn.h"

#include <cstdio>

#include "ui/ui_root.h"

namespace ui {
namespace {

lv_obj_t* s_screen = nullptr;
lv_obj_t* s_title = nullptr;
lv_obj_t* s_body = nullptr;
lv_obj_t* s_bar = nullptr;      // progress bar (RECORDING)
lv_obj_t* s_primary = nullptr;  // Start / Save
lv_obj_t* s_primary_lbl = nullptr;
lv_obj_t* s_secondary = nullptr;  // Cancel / Redo
lv_obj_t* s_secondary_lbl = nullptr;
lv_obj_t* s_stain_sw = nullptr;   // DONE step: mark this pan as stainless
lv_obj_t* s_stain_lbl = nullptr;

void primary_cb(lv_event_t*) {
  // Start (from OFF) or Save (from DONE) — the controller reads the phase.
  ui::learn_cmd(1);   // 1 = start-or-save (main disambiguates by phase)
}
void secondary_cb(lv_event_t*) { ui::learn_cmd(2); }  // cancel / redo
void done_cb(lv_event_t*) { ui::show_home(); }
void pans_cb(lv_event_t*) { ui::show_profiles(); }

}  // namespace

lv_obj_t* learn_create() {
  if (s_screen) return s_screen;
  lv_obj_t* scr = lv_obj_create(nullptr);
  s_screen = scr;
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  s_title = lv_label_create(scr);
  lv_label_set_text(s_title, "Learn Pan Mode");
  lv_obj_set_style_text_font(s_title, &lv_font_montserrat_28, 0);
  lv_obj_align(s_title, LV_ALIGN_TOP_MID, 0, 16);

  s_body = lv_label_create(scr);
  lv_obj_set_style_text_font(s_body, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(s_body, lv_color_hex(0xB7C0CC), 0);
  lv_obj_set_width(s_body, 440);
  lv_obj_set_style_text_align(s_body, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(s_body, LV_ALIGN_CENTER, 0, -20);

  s_bar = lv_bar_create(scr);
  lv_obj_set_size(s_bar, 360, 16);
  lv_obj_align(s_bar, LV_ALIGN_CENTER, 0, 30);
  lv_bar_set_range(s_bar, 0, 100);

  s_primary = lv_btn_create(scr);
  lv_obj_set_size(s_primary, 150, 52);
  lv_obj_align(s_primary, LV_ALIGN_BOTTOM_MID, -84, -12);
  lv_obj_set_style_bg_color(s_primary, lv_color_hex(0x2E7D32), 0);
  lv_obj_add_event_cb(s_primary, primary_cb, LV_EVENT_CLICKED, nullptr);
  s_primary_lbl = lv_label_create(s_primary);
  lv_obj_center(s_primary_lbl);

  s_secondary = lv_btn_create(scr);
  lv_obj_set_size(s_secondary, 150, 52);
  lv_obj_align(s_secondary, LV_ALIGN_BOTTOM_MID, 84, -12);
  lv_obj_set_style_bg_color(s_secondary, lv_color_hex(0x2A323C), 0);
  lv_obj_add_event_cb(s_secondary, secondary_cb, LV_EVENT_CLICKED, nullptr);
  s_secondary_lbl = lv_label_create(s_secondary);
  lv_obj_center(s_secondary_lbl);

  lv_obj_t* done = lv_btn_create(scr);
  lv_obj_set_size(done, 84, 30);
  lv_obj_align(done, LV_ALIGN_TOP_RIGHT, -8, 6);
  lv_obj_add_event_cb(done, done_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* dl = lv_label_create(done);
  lv_label_set_text(dl, "Done");
  lv_obj_center(dl);

  lv_obj_t* pans = lv_btn_create(scr);
  lv_obj_set_size(pans, 120, 30);
  lv_obj_align(pans, LV_ALIGN_TOP_LEFT, 8, 6);
  lv_obj_set_style_bg_color(pans, lv_color_hex(0x2A323C), 0);
  lv_obj_add_event_cb(pans, pans_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* pl = lv_label_create(pans);
  lv_label_set_text(pl, LV_SYMBOL_LIST " My Pans");
  lv_obj_center(pl);

  // DONE step only: record the pan's material with its profile — the selected
  // pan (not a preset) is what drives the stainless guidance behavior.
  s_stain_sw = lv_switch_create(scr);
  lv_obj_align(s_stain_sw, LV_ALIGN_CENTER, -70, 78);
  lv_obj_add_flag(s_stain_sw, LV_OBJ_FLAG_HIDDEN);
  s_stain_lbl = lv_label_create(scr);
  lv_label_set_text(s_stain_lbl, "Stainless pan");
  lv_obj_set_style_text_font(s_stain_lbl, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_stain_lbl, lv_color_hex(0xB7C0CC), 0);
  lv_obj_align(s_stain_lbl, LV_ALIGN_CENTER, 30, 78);
  lv_obj_add_flag(s_stain_lbl, LV_OBJ_FLAG_HIDDEN);
  return s_screen;
}

void learn_update(const UiState& s) {
  if (!s_screen) learn_create();
  char buf[64];
  const bool done = s.learnPhase == LearnPhase::DONE;
  done ? lv_obj_clear_flag(s_stain_sw, LV_OBJ_FLAG_HIDDEN)
       : lv_obj_add_flag(s_stain_sw, LV_OBJ_FLAG_HIDDEN);
  done ? lv_obj_clear_flag(s_stain_lbl, LV_OBJ_FLAG_HIDDEN)
       : lv_obj_add_flag(s_stain_lbl, LV_OBJ_FLAG_HIDDEN);
  switch (s.learnPhase) {
    case LearnPhase::RECORDING:
      lv_label_set_text(s_body,
                        "Learning... keep the burner steady on MEDIUM");
      lv_bar_set_value(s_bar, s.learnProgress, LV_ANIM_OFF);
      lv_obj_clear_flag(s_bar, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(s_primary, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(s_secondary_lbl, "Cancel");
      break;
    case LearnPhase::DONE:
      std::snprintf(buf, sizeof(buf), "Learned this pan.\nThermal lag: %.1f min",
                    s.learnedLagMinutes);
      lv_label_set_text(s_body, buf);
      lv_obj_add_flag(s_bar, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(s_primary, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(s_primary_lbl, "Save");
      lv_label_set_text(s_secondary_lbl, "Redo");
      break;
    default:  // OFF / intro
      lv_label_set_text(s_body,
                        "Put an empty pan on. Set the burner to MEDIUM.\n"
                        "Tap Start, then leave it to heat.");
      lv_obj_add_flag(s_bar, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(s_primary, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(s_primary_lbl, "Start");
      lv_label_set_text(s_secondary_lbl, "Cancel");
      break;
  }
}

bool learn_get_stainless() {
  return s_stain_sw && lv_obj_has_state(s_stain_sw, LV_STATE_CHECKED);
}

}  // namespace ui
