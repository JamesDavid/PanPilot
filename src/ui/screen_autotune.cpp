// screen_autotune.cpp — see screen_autotune.h.
#include "screen_autotune.h"

#include <cstdio>

#include "ui/ui_root.h"
#include "core/control/autotune.h"

namespace ui {
namespace {
lv_obj_t* s_screen = nullptr;
lv_obj_t* s_title = nullptr;
lv_obj_t* s_body = nullptr;
lv_obj_t* s_bar = nullptr;        // progress bar (running)
lv_obj_t* s_gains = nullptr;      // result gains
lv_obj_t* s_primary = nullptr;    // Start / Save
lv_obj_t* s_primary_lbl = nullptr;
lv_obj_t* s_secondary_lbl = nullptr;  // Cancel / Discard
uint8_t s_state = 0;

void primary_cb(lv_event_t*) {
  if (s_state == 0) ui::autotune_cmd(0);       // Start
  else if (s_state == 2) ui::autotune_cmd(1);  // Save
}
void secondary_cb(lv_event_t*) { ui::autotune_cmd(2); }  // Cancel / Discard / back
}  // namespace

lv_obj_t* autotune_create() {
  if (s_screen) return s_screen;
  lv_obj_t* scr = lv_obj_create(nullptr);
  s_screen = scr;
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  s_title = lv_label_create(scr);
  lv_label_set_text(s_title, "PID Autotune");
  lv_obj_set_style_text_font(s_title, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(s_title, lv_color_hex(0xF5F5F5), 0);
  lv_obj_align(s_title, LV_ALIGN_TOP_MID, 0, 34);

  s_body = lv_label_create(scr);
  lv_label_set_long_mode(s_body, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(s_body, 440);
  lv_obj_set_style_text_align(s_body, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(s_body, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_body, lv_color_hex(0xB7BEC8), 0);
  lv_obj_align(s_body, LV_ALIGN_TOP_MID, 0, 84);

  s_bar = lv_bar_create(scr);
  lv_obj_set_size(s_bar, 360, 18);
  lv_obj_align(s_bar, LV_ALIGN_CENTER, 0, 0);
  lv_bar_set_range(s_bar, 0, RelayAutotuner::TARGET_CYCLES);
  lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x2A323C), LV_PART_MAIN);
  lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x2E5AAC), LV_PART_INDICATOR);
  lv_obj_add_flag(s_bar, LV_OBJ_FLAG_HIDDEN);

  s_gains = lv_label_create(scr);
  lv_obj_set_style_text_font(s_gains, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(s_gains, lv_color_hex(0x5AA0FF), 0);
  lv_obj_set_style_text_align(s_gains, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(s_gains, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(s_gains, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t* sec = lv_btn_create(scr);
  lv_obj_set_size(sec, 150, 44);
  lv_obj_align(sec, LV_ALIGN_BOTTOM_LEFT, 12, -10);
  lv_obj_set_style_bg_color(sec, lv_color_hex(0x2A323C), 0);
  lv_obj_add_event_cb(sec, secondary_cb, LV_EVENT_CLICKED, nullptr);
  s_secondary_lbl = lv_label_create(sec);
  lv_obj_set_style_text_font(s_secondary_lbl, &lv_font_montserrat_20, 0);
  lv_obj_center(s_secondary_lbl);

  s_primary = lv_btn_create(scr);
  lv_obj_set_size(s_primary, 180, 44);
  lv_obj_align(s_primary, LV_ALIGN_BOTTOM_RIGHT, -12, -10);
  lv_obj_set_style_bg_color(s_primary, lv_color_hex(0x2E7D32), 0);
  lv_obj_add_event_cb(s_primary, primary_cb, LV_EVENT_CLICKED, nullptr);
  s_primary_lbl = lv_label_create(s_primary);
  lv_obj_set_style_text_font(s_primary_lbl, &lv_font_montserrat_20, 0);
  lv_obj_center(s_primary_lbl);
  return s_screen;
}

void autotune_update(const UiState& s) {
  if (!s_screen) autotune_create();
  s_state = s.autotuneState;
  char b[96];

  if (s_state == 1) {                          // running
    lv_label_set_text(s_title, "Tuning\xE2\x80\xA6");
    lv_label_set_text(s_body,
        "Keep the pan on the burner. PanPilot is pulsing the heat to measure how "
        "your setup responds. This takes a few minutes.");
    lv_obj_clear_flag(s_bar, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_gains, LV_OBJ_FLAG_HIDDEN);
    lv_bar_set_value(s_bar, s.autotuneProgress, LV_ANIM_OFF);
    lv_label_set_text(s_primary_lbl, "Working\xE2\x80\xA6");
    lv_obj_add_state(s_primary, LV_STATE_DISABLED);
    lv_label_set_text(s_secondary_lbl, "Stop");
  } else if (s_state == 2) {                   // done
    lv_label_set_text(s_title, "Tuned!");
    lv_label_set_text(s_body, "New gains from the measured response:");
    lv_obj_add_flag(s_bar, LV_OBJ_FLAG_HIDDEN);
    std::snprintf(b, sizeof(b), "Kp %.3f    Ki %.4f    Kd %.3f",
                  s.autotuneKp, s.autotuneKi, s.autotuneKd);
    lv_label_set_text(s_gains, b);
    lv_obj_clear_flag(s_gains, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(s_primary_lbl, "Save");
    lv_obj_clear_state(s_primary, LV_STATE_DISABLED);
    lv_label_set_text(s_secondary_lbl, "Discard");
  } else {                                     // idle
    lv_label_set_text(s_title, "PID Autotune");
    lv_obj_add_flag(s_bar, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_gains, LV_OBJ_FLAG_HIDDEN);
    if (!s.assistArmed) {
      lv_label_set_text(s_body,
          "Arm Autopilot first (Settings -> Autopilot). Autotune pulses the "
          "burner through the actuator, so it needs control authority.");
      lv_obj_add_state(s_primary, LV_STATE_DISABLED);
    } else {
      lv_label_set_text(s_body,
          "PanPilot will briefly pulse the burner to learn how it heats, then "
          "compute PID gains for tighter holds. Keep an empty pan on the heat.");
      lv_obj_clear_state(s_primary, LV_STATE_DISABLED);
    }
    lv_label_set_text(s_primary_lbl, LV_SYMBOL_PLAY "  Start");
    lv_label_set_text(s_secondary_lbl, "Back");
  }
}

}  // namespace ui
