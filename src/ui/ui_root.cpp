// ui_root.cpp — M0 bring-up screen: title, version, and a BEEP button that
// exercises the buzzer (M0 accept: "touch a button, hear a beep"). Grows into
// the real home/thermal/preset screens across M1..M6.
#include "ui_root.h"

#include <lvgl.h>
#include "hal/buzzer.h"
#include "app_config.h"

namespace ui {
namespace {

void beep_event_cb(lv_event_t* e) {
  (void)e;
  hal::buzzer_play(hal::BuzzPattern::Chirp);
}

}  // namespace

void root_init() {
  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);

  // Title
  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "PanPilot");
  lv_obj_set_style_text_color(title, lv_color_hex(0xF5F5F5), LV_PART_MAIN);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_48, LV_PART_MAIN);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

  // Version / subtitle
  lv_obj_t* ver = lv_label_create(scr);
  lv_label_set_text(ver, "M0 bring-up  -  " PANPILOT_FW_VERSION);
  lv_obj_set_style_text_color(ver, lv_color_hex(0x8A93A0), LV_PART_MAIN);
  lv_obj_align_to(ver, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

  // BEEP button (min 60x60 touch target, base spec §9)
  lv_obj_t* btn = lv_btn_create(scr);
  lv_obj_set_size(btn, 220, 96);
  lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0x2E7D32), LV_PART_MAIN);
  lv_obj_set_style_radius(btn, 16, LV_PART_MAIN);
  lv_obj_add_event_cb(btn, beep_event_cb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t* btn_lbl = lv_label_create(btn);
  lv_label_set_text(btn_lbl, "BEEP");
  lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_28, LV_PART_MAIN);
  lv_obj_center(btn_lbl);
}

}  // namespace ui
