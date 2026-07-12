// screen_settings.cpp — see screen_settings.h.
#include "screen_settings.h"

#include <cstring>

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
lv_obj_t* s_val_stainless = nullptr;
lv_obj_t* s_val_wifi = nullptr;

// Time-zone picker (own screen): a tap-to-cycle row inside a scrollable list
// was bench-found flaky — taps kept registering as scroll-drags. The row now
// opens a plain scrollable list to pick from (current zone checkmarked).
lv_obj_t* s_tz_screen = nullptr;
lv_obj_t* s_tz_list = nullptr;
uint8_t s_tz_current = 0;

void done_cb(lv_event_t*) { ui::show_home(); }
void unit_cb(lv_event_t*) { ui::settings_toggle_unit(); }
void sound_cb(lv_event_t*) { ui::settings_toggle_mute(); }
void bright_cb(lv_event_t*) { ui::settings_cycle_brightness(); }
void stainless_cb(lv_event_t*) { ui::settings_toggle_stainless(); }
void assist_cb(lv_event_t*) { ui::show_assist_arm(); }
void autotune_cb(lv_event_t*) { ui::show_autotune(); }
void wifi_cb(lv_event_t*) { ui::settings_wifi_tap(); }

void tz_pick_cb(lv_event_t* e) {
  ui::settings_set_timezone((uint8_t)(uintptr_t)lv_event_get_user_data(e));
  lv_scr_load(s_screen);            // back to Settings, new zone shown
}
void tz_back_cb(lv_event_t*) { lv_scr_load(s_screen); }

void tz_picker_build();             // fwd
void tz_cb(lv_event_t*) {           // the Settings row opens the picker
  tz_picker_build();
  lv_scr_load(s_tz_screen);
}

void tz_picker_build() {
  if (!s_tz_screen) {
    s_tz_screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(s_tz_screen, lv_color_hex(0x101418), LV_PART_MAIN);
    lv_obj_clear_flag(s_tz_screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(s_tz_screen);
    lv_label_set_text(title, "Time zone");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x8A93A0), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 12, 10);

    lv_obj_t* back = lv_btn_create(s_tz_screen);
    lv_obj_set_size(back, 84, 30);
    lv_obj_align(back, LV_ALIGN_TOP_RIGHT, -8, 6);
    lv_obj_add_event_cb(back, tz_back_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* bl = lv_label_create(back);
    lv_label_set_text(bl, "Back");
    lv_obj_center(bl);

    s_tz_list = lv_list_create(s_tz_screen);
    lv_obj_set_size(s_tz_list, 468, 272);
    lv_obj_align(s_tz_list, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(s_tz_list, lv_color_hex(0x101418), 0);
    lv_obj_set_style_pad_row(s_tz_list, 4, 0);
  }
  lv_obj_clean(s_tz_list);          // rebuild so the checkmark tracks current
  for (int i = 0; i < tz_count(); ++i) {
    lv_obj_t* row = lv_btn_create(s_tz_list);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 44);
    const bool cur = (i == (int)s_tz_current);
    lv_obj_set_style_bg_color(row, lv_color_hex(cur ? 0x2E5AAC : 0x1A2027), 0);
    lv_obj_set_style_radius(row, 8, 0);
    lv_obj_add_event_cb(row, tz_pick_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
    lv_obj_t* l = lv_label_create(row);
    char b[40];
    lv_snprintf(b, sizeof(b), "%s%s", cur ? LV_SYMBOL_OK "  " : "", tz_name(i));
    lv_label_set_text(l, b);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
    lv_obj_align(l, LV_ALIGN_LEFT_MID, 8, 0);
  }
}

// One full-width settings row ("Name .... value", whole row taps). The layout
// went tile-grid for one round to kill scroll-bonking, but 8 tiles in 278 px
// read as "smooshed" (bench 2026-07-12) — the verdict was: full-size rows,
// ~90% width, with a FAT always-visible scrollbar owning the right edge, and
// the global 6 px scroll threshold doing the tap-vs-scroll discrimination.
lv_obj_t* mk_row(lv_obj_t* p, const char* name, lv_event_cb_t cb) {
  lv_obj_t* row = lv_btn_create(p);
  lv_obj_set_size(row, 424, 56);   // ~90% of the container; scrollbar gets the rest
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
  lv_obj_align(v, LV_ALIGN_RIGHT_MID, -10, 0);
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
  // Fat, always-visible scrollbar owning the right edge (bench 2026-07-12:
  // "90% button width, 10% scrollbar" — the bar doubles as a drag handle and
  // as the visual cue that the list scrolls at all).
  lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ON);
  lv_obj_set_style_width(list, 20, LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_color(list, lv_color_hex(0x3A4552), LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_opa(list, LV_OPA_70, LV_PART_SCROLLBAR);
  lv_obj_set_style_radius(list, 6, LV_PART_SCROLLBAR);

  s_val_unit = mk_row(list, "Temperature", unit_cb);
  s_val_sound = mk_row(list, "Sound", sound_cb);
  s_val_bright = mk_row(list, "Brightness", bright_cb);
  // Pan material, not a preset (design decision 2026-07-11): applies to every
  // preset/food; ORed with the reflective-stainless auto-detection.
  s_val_stainless = mk_row(list, "Stainless pan", stainless_cb);
  s_val_tz = mk_row(list, "Time zone", tz_cb);
#if defined(ENABLE_WIFI) || defined(PANPILOT_SIM)
  // Wi-Fi: shows the join address once connected; when unprovisioned, tapping
  // reopens the captive-portal AP (bench 2026-07-11: the first-boot portal was
  // undiscoverable — nothing on screen named it, and after its 3-min timeout
  // there was no way back in).
  s_val_wifi = mk_row(list, "Wi-Fi", wifi_cb);
  lv_label_set_text(s_val_wifi, "tap to set up");
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
                     uint8_t tzIndex, bool stainlessPan) {
  if (!s_screen) settings_create();
  // Dirty-check. This runs on every 4 Hz UI tick while Settings is open (so
  // web/MQTT-side changes mirror in live), but lv_label_set_text invalidates
  // the label even for identical text — five forced redraw regions per tick
  // on top of the scroll container made scrolling visibly laggy (bench
  // 2026-07-11). At steady state this now touches nothing.
  static bool have = false;
  static bool pF, pM, pS;
  static uint8_t pB, pT;
  if (have && pF == useF && pM == muted && pB == brightnessLevel &&
      pT == tzIndex && pS == stainlessPan)
    return;
  have = true;
  pF = useF; pM = muted; pB = brightnessLevel; pT = tzIndex; pS = stainlessPan;
  s_tz_current = tzIndex;
  lv_label_set_text(s_val_unit, useF ? "\xC2\xB0" "F" : "\xC2\xB0" "C");
  lv_label_set_text(s_val_sound, muted ? "Muted" : "On");
  lv_label_set_text(s_val_bright, panpilot::brightness_name(brightnessLevel));
  lv_label_set_text(s_val_tz, tz_name(tzIndex));
  lv_label_set_text(s_val_stainless, stainlessPan ? "Yes" : "No");
}

void settings_set_wifi(const char* line) {
  if (!s_val_wifi || !line) return;
  if (std::strcmp(lv_label_get_text(s_val_wifi), line) == 0) return;  // no
  lv_label_set_text(s_val_wifi, line);  // invalidation unless it changed
}

}  // namespace ui
