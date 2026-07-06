// screen_assist.cpp — see screen_assist.h.
#include "screen_assist.h"

#include <cstdio>

#include "ui/ui_root.h"

namespace ui {
namespace {
lv_obj_t* s_screen = nullptr;
lv_obj_t* s_actuator = nullptr;   // actuator name line
lv_obj_t* s_switch = nullptr;     // "I'll stay nearby"
lv_obj_t* s_arm = nullptr;        // ARM button (gated on the switch)
lv_obj_t* s_arm_lbl = nullptr;
bool s_ready = false;             // actuator configured + alive

void refresh_arm() {
  const bool ok = s_ready && lv_obj_has_state(s_switch, LV_STATE_CHECKED);
  if (ok) {
    lv_obj_clear_state(s_arm, LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(s_arm, lv_color_hex(0xE07000), 0);
  } else {
    lv_obj_add_state(s_arm, LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(s_arm, lv_color_hex(0x3A4048), 0);
  }
}

void switch_cb(lv_event_t*) { refresh_arm(); }
void cancel_cb(lv_event_t*) { ui::show_home(); }
void arm_cb(lv_event_t*) {
  if (s_ready && lv_obj_has_state(s_switch, LV_STATE_CHECKED)) ui::assist_arm();
}

void bullet(lv_obj_t* p, const char* txt, int y) {
  lv_obj_t* l = lv_label_create(p);
  lv_label_set_text(l, txt);
  lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(l, 452);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(0xC7CDD5), 0);
  lv_obj_align(l, LV_ALIGN_TOP_LEFT, 14, y);
}
}  // namespace

lv_obj_t* assist_create() {
  if (s_screen) return s_screen;
  lv_obj_t* scr = lv_obj_create(nullptr);
  s_screen = scr;
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, LV_SYMBOL_WARNING "  Arm Autopilot");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0xE07000), 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 12, 10);

  s_actuator = lv_label_create(scr);
  lv_obj_set_style_text_font(s_actuator, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_actuator, lv_color_hex(0x8A93A0), 0);
  lv_obj_align(s_actuator, LV_ALIGN_TOP_LEFT, 14, 48);

  bullet(scr, LV_SYMBOL_RIGHT " PanPilot will control the burner power. You are "
              "still the cook - stay in the kitchen.", 74);
  bullet(scr, LV_SYMBOL_RIGHT " Interlocks cut power on low confidence, overheat, "
              "a lost pan, or a sensor/comms fault.", 112);
  bullet(scr, LV_SYMBOL_RIGHT " Press the red STOP bar any time to cut power "
              "instantly.", 150);

  s_switch = lv_switch_create(scr);
  lv_obj_align(s_switch, LV_ALIGN_TOP_LEFT, 14, 196);
  lv_obj_add_event_cb(s_switch, switch_cb, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_t* sw_l = lv_label_create(scr);
  lv_label_set_text(sw_l, "I'll stay nearby while it runs");
  lv_obj_set_style_text_font(sw_l, &lv_font_montserrat_20, 0);
  lv_obj_align(sw_l, LV_ALIGN_TOP_LEFT, 80, 198);

  lv_obj_t* cancel = lv_btn_create(scr);
  lv_obj_set_size(cancel, 150, 44);
  lv_obj_align(cancel, LV_ALIGN_BOTTOM_LEFT, 12, -10);
  lv_obj_set_style_bg_color(cancel, lv_color_hex(0x2A323C), 0);
  lv_obj_add_event_cb(cancel, cancel_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* cl = lv_label_create(cancel);
  lv_label_set_text(cl, "Cancel");
  lv_obj_set_style_text_font(cl, &lv_font_montserrat_20, 0);
  lv_obj_center(cl);

  s_arm = lv_btn_create(scr);
  lv_obj_set_size(s_arm, 180, 44);
  lv_obj_align(s_arm, LV_ALIGN_BOTTOM_RIGHT, -12, -10);
  lv_obj_add_event_cb(s_arm, arm_cb, LV_EVENT_CLICKED, nullptr);
  s_arm_lbl = lv_label_create(s_arm);
  lv_label_set_text(s_arm_lbl, LV_SYMBOL_POWER "  ARM");
  lv_obj_set_style_text_font(s_arm_lbl, &lv_font_montserrat_20, 0);
  lv_obj_center(s_arm_lbl);
  return s_screen;
}

void assist_load(const char* actuatorName, bool actuatorReady) {
  if (!s_screen) assist_create();
  s_ready = actuatorReady;
  char b[64];
  if (actuatorReady)
    std::snprintf(b, sizeof(b), "Actuator: %s (ready)",
                  actuatorName && actuatorName[0] ? actuatorName : "actuator");
  else
    std::snprintf(b, sizeof(b), "No actuator configured - advisory only");
  lv_label_set_text(s_actuator, b);
  lv_obj_clear_state(s_switch, LV_STATE_CHECKED);
  refresh_arm();
}

}  // namespace ui
