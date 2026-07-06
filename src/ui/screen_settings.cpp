// screen_settings.cpp — see screen_settings.h.
#include "screen_settings.h"

#include "ui/ui_root.h"
#include "app_config.h"
#include "core/settings.h"

namespace ui {
namespace {
lv_obj_t* s_screen = nullptr;
lv_obj_t* s_val_unit = nullptr;
lv_obj_t* s_val_sound = nullptr;
lv_obj_t* s_val_bright = nullptr;

void done_cb(lv_event_t*) { ui::show_home(); }
void unit_cb(lv_event_t*) { ui::settings_toggle_unit(); }
void sound_cb(lv_event_t*) { ui::settings_toggle_mute(); }
void bright_cb(lv_event_t*) { ui::settings_cycle_brightness(); }

// One "Label ............ value" row that fires cb on tap. Returns the value
// label so settings_update() can refresh it.
lv_obj_t* mk_row(lv_obj_t* p, const char* name, lv_event_cb_t cb, int y) {
  lv_obj_t* row = lv_btn_create(p);
  lv_obj_set_size(row, 456, 52);
  lv_obj_align(row, LV_ALIGN_TOP_MID, 0, y);
  lv_obj_set_style_bg_color(row, lv_color_hex(0x1A2027), 0);
  lv_obj_set_style_radius(row, 10, 0);
  lv_obj_add_event_cb(row, cb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t* nm = lv_label_create(row);
  lv_label_set_text(nm, name);
  lv_obj_set_style_text_font(nm, &lv_font_montserrat_20, 0);
  lv_obj_align(nm, LV_ALIGN_LEFT_MID, 8, 0);

  lv_obj_t* v = lv_label_create(row);
  lv_obj_set_style_text_font(v, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(v, lv_color_hex(0x5AA0FF), 0);
  lv_obj_align(v, LV_ALIGN_RIGHT_MID, -12, 0);
  return v;
}
}  // namespace

lv_obj_t* settings_create() {
  if (s_screen) return s_screen;
  lv_obj_t* scr = lv_obj_create(nullptr);
  s_screen = scr;
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "Settings");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x8A93A0), 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 12, 10);

  lv_obj_t* done = lv_btn_create(scr);
  lv_obj_set_size(done, 84, 30);
  lv_obj_align(done, LV_ALIGN_TOP_RIGHT, -8, 6);
  lv_obj_add_event_cb(done, done_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* dl = lv_label_create(done);
  lv_label_set_text(dl, "Done");
  lv_obj_center(dl);

  s_val_unit = mk_row(scr, "Temperature", unit_cb, 44);
  s_val_sound = mk_row(scr, "Sound", sound_cb, 102);
  s_val_bright = mk_row(scr, "Brightness", bright_cb, 160);

  lv_obj_t* about = lv_label_create(scr);
  lv_label_set_text(about, "PanPilot  " PANPILOT_FW_VERSION "     tap a row to change");
  lv_obj_set_style_text_font(about, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(about, lv_color_hex(0x5A626C), 0);
  lv_obj_align(about, LV_ALIGN_BOTTOM_MID, 0, -14);
  return s_screen;
}

void settings_update(bool useF, bool muted, uint8_t brightnessLevel) {
  if (!s_screen) settings_create();
  lv_label_set_text(s_val_unit, useF ? "\xC2\xB0" "F" : "\xC2\xB0" "C");
  lv_label_set_text(s_val_sound, muted ? "Muted" : "On");
  lv_label_set_text(s_val_bright, panpilot::brightness_name(brightnessLevel));
}

}  // namespace ui
