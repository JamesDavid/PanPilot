// screen_settings.cpp — see screen_settings.h.
#include "screen_settings.h"

#include "ui/ui_root.h"
#include "app_config.h"
#include "core/settings.h"
#include "core/timezones.h"

namespace ui {
namespace {
lv_obj_t* s_screen = nullptr;
lv_obj_t* s_val_unit = nullptr;
lv_obj_t* s_val_sound = nullptr;
lv_obj_t* s_val_bright = nullptr;
lv_obj_t* s_val_tz = nullptr;

void done_cb(lv_event_t*) { ui::show_home(); }
void unit_cb(lv_event_t*) { ui::settings_toggle_unit(); }
void sound_cb(lv_event_t*) { ui::settings_toggle_mute(); }
void bright_cb(lv_event_t*) { ui::settings_cycle_brightness(); }
void tz_cb(lv_event_t*) { ui::settings_cycle_timezone(); }
void assist_cb(lv_event_t*) { ui::show_assist_arm(); }
void autotune_cb(lv_event_t*) { ui::show_autotune(); }

// One "Label ............ value" row (added to a scroll container) that fires cb
// on tap. Returns the value label so settings_update() can refresh it.
lv_obj_t* mk_row(lv_obj_t* p, const char* name, lv_event_cb_t cb) {
  lv_obj_t* row = lv_btn_create(p);
  lv_obj_set_width(row, lv_pct(100));
  lv_obj_set_height(row, 50);
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

  // Scrollable list of setting rows (more rows than fit swipe into view).
  lv_obj_t* list = lv_obj_create(scr);
  lv_obj_set_size(list, 476, 278);
  lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 40);
  lv_obj_set_style_bg_opa(list, LV_OPA_0, 0);
  lv_obj_set_style_border_width(list, 0, 0);
  lv_obj_set_style_pad_all(list, 6, 0);
  lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(list, 6, 0);
  lv_obj_set_scroll_dir(list, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);

  s_val_unit = mk_row(list, "Temperature", unit_cb);
  s_val_sound = mk_row(list, "Sound", sound_cb);
  s_val_bright = mk_row(list, "Brightness", bright_cb);
  s_val_tz = mk_row(list, "Time zone", tz_cb);
#if defined(ENABLE_WIFI) || defined(PANPILOT_SIM)
  // Autopilot + autotune need the MQTT actuator — no point offering them (or
  // paying their screens' LVGL heap) on the Wi-Fi-less basic build. The sim
  // keeps them so the screenshots show the full set.
  lv_obj_t* av = mk_row(list, "Autopilot", assist_cb);
  lv_label_set_text(av, "Set up  " LV_SYMBOL_RIGHT);
  lv_obj_t* at = mk_row(list, "PID autotune", autotune_cb);
  lv_label_set_text(at, "Tune  " LV_SYMBOL_RIGHT);
#endif
  return s_screen;
}

void settings_update(bool useF, bool muted, uint8_t brightnessLevel,
                     uint8_t tzIndex) {
  if (!s_screen) settings_create();
  lv_label_set_text(s_val_unit, useF ? "\xC2\xB0" "F" : "\xC2\xB0" "C");
  lv_label_set_text(s_val_sound, muted ? "Muted" : "On");
  lv_label_set_text(s_val_bright, panpilot::brightness_name(brightnessLevel));
  lv_label_set_text(s_val_tz, tz_name(tzIndex));
}

}  // namespace ui
