// screen_preset_edit.cpp — see screen_preset_edit.h.
#include "screen_preset_edit.h"

#include <cstdio>
#include <cstring>

#include "ui/ui_root.h"
#include "core/presets.h"

namespace ui {
namespace {
lv_obj_t* s_screen = nullptr;
lv_obj_t* s_title = nullptr;
lv_obj_t* s_name = nullptr;      // textarea
lv_obj_t* s_kb = nullptr;        // keyboard (hidden until name tapped)
lv_obj_t* s_lo_lbl = nullptr;
lv_obj_t* s_hi_lbl = nullptr;
lv_obj_t* s_stain = nullptr;     // switch
lv_obj_t* s_del = nullptr;       // Delete button (edit only)

int s_lo = 300, s_hi = 350;
const int STEP = 25, LO_MIN = 100, HI_MAX = 650;

void refresh_temps() {
  char b[16];
  std::snprintf(b, sizeof(b), "%d\xC2\xB0" "F", s_lo);
  lv_label_set_text(s_lo_lbl, b);
  std::snprintf(b, sizeof(b), "%d\xC2\xB0" "F", s_hi);
  lv_label_set_text(s_hi_lbl, b);
}

void lo_minus(lv_event_t*) { if (s_lo - STEP >= LO_MIN) s_lo -= STEP; refresh_temps(); }
void lo_plus(lv_event_t*)  { if (s_lo + STEP < s_hi) s_lo += STEP; refresh_temps(); }
void hi_minus(lv_event_t*) { if (s_hi - STEP > s_lo) s_hi -= STEP; refresh_temps(); }
void hi_plus(lv_event_t*)  { if (s_hi + STEP <= HI_MAX) s_hi += STEP; refresh_temps(); }

void name_focus(lv_event_t*) { lv_obj_clear_flag(s_kb, LV_OBJ_FLAG_HIDDEN); }
void kb_done(lv_event_t*) { lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN); }

void cancel_cb(lv_event_t*) { ui::show_presets(); }
void delete_cb(lv_event_t*) { ui::preset_edit_delete(); }
void save_cb(lv_event_t*) {
  const char* nm = lv_textarea_get_text(s_name);
  const bool stain = lv_obj_has_state(s_stain, LV_STATE_CHECKED);
  ui::preset_edit_save(nm, s_lo, s_hi, stain);
}

// A "[-]  value  [+]" temperature stepper row. Returns the value label.
lv_obj_t* mk_stepper(lv_obj_t* p, const char* caption, int y,
                     lv_event_cb_t minus, lv_event_cb_t plus) {
  lv_obj_t* cap = lv_label_create(p);
  lv_label_set_text(cap, caption);
  lv_obj_set_style_text_font(cap, &lv_font_montserrat_20, 0);
  lv_obj_align(cap, LV_ALIGN_TOP_LEFT, 16, y + 8);

  auto mk = [&](const char* t, lv_event_cb_t cb, int x) {
    lv_obj_t* b = lv_btn_create(p);
    lv_obj_set_size(b, 48, 40);
    lv_obj_align(b, LV_ALIGN_TOP_RIGHT, x, y);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x2A323C), 0);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, t);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
    lv_obj_center(l);
  };
  mk("-", minus, -140);
  mk("+", plus, -16);
  lv_obj_t* v = lv_label_create(p);
  lv_obj_set_style_text_font(v, &lv_font_montserrat_20, 0);
  lv_obj_align(v, LV_ALIGN_TOP_RIGHT, -70, y + 8);
  lv_obj_set_style_text_align(v, LV_TEXT_ALIGN_CENTER, 0);
  return v;
}
}  // namespace

lv_obj_t* preset_edit_create() {
  if (s_screen) return s_screen;
  lv_obj_t* scr = lv_obj_create(nullptr);
  s_screen = scr;
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  s_title = lv_label_create(scr);
  lv_label_set_text(s_title, "New preset");
  lv_obj_set_style_text_font(s_title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(s_title, lv_color_hex(0x8A93A0), 0);
  lv_obj_align(s_title, LV_ALIGN_TOP_LEFT, 12, 10);

  lv_obj_t* save = lv_btn_create(scr);
  lv_obj_set_size(save, 84, 30);
  lv_obj_align(save, LV_ALIGN_TOP_RIGHT, -8, 6);
  lv_obj_set_style_bg_color(save, lv_color_hex(0x2E7D32), 0);
  lv_obj_add_event_cb(save, save_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* sl = lv_label_create(save);
  lv_label_set_text(sl, "Save");
  lv_obj_center(sl);

  lv_obj_t* nl = lv_label_create(scr);
  lv_label_set_text(nl, "Name");
  lv_obj_set_style_text_font(nl, &lv_font_montserrat_20, 0);
  lv_obj_align(nl, LV_ALIGN_TOP_LEFT, 16, 52);

  s_name = lv_textarea_create(scr);
  lv_textarea_set_one_line(s_name, true);
  lv_textarea_set_max_length(s_name, PRESET_NAME_MAX);
  lv_textarea_set_placeholder_text(s_name, "e.g. Smash burger");
  lv_obj_set_size(s_name, 320, 44);
  lv_obj_align(s_name, LV_ALIGN_TOP_RIGHT, -12, 46);
  lv_obj_add_event_cb(s_name, name_focus, LV_EVENT_CLICKED, nullptr);

  s_lo_lbl = mk_stepper(scr, "Low", 110, lo_minus, lo_plus);
  s_hi_lbl = mk_stepper(scr, "High", 158, hi_minus, hi_plus);

  lv_obj_t* stl = lv_label_create(scr);
  lv_label_set_text(stl, "Stainless pan");
  lv_obj_set_style_text_font(stl, &lv_font_montserrat_20, 0);
  lv_obj_align(stl, LV_ALIGN_TOP_LEFT, 16, 214);
  s_stain = lv_switch_create(scr);
  lv_obj_align(s_stain, LV_ALIGN_TOP_RIGHT, -16, 210);

  lv_obj_t* cancel = lv_btn_create(scr);
  lv_obj_set_size(cancel, 120, 34);
  lv_obj_align(cancel, LV_ALIGN_BOTTOM_LEFT, 12, -8);
  lv_obj_set_style_bg_color(cancel, lv_color_hex(0x2A323C), 0);
  lv_obj_add_event_cb(cancel, cancel_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* cl = lv_label_create(cancel);
  lv_label_set_text(cl, "Cancel");
  lv_obj_center(cl);

  s_del = lv_btn_create(scr);
  lv_obj_set_size(s_del, 120, 34);
  lv_obj_align(s_del, LV_ALIGN_BOTTOM_RIGHT, -12, -8);
  lv_obj_set_style_bg_color(s_del, lv_color_hex(0xC62828), 0);
  lv_obj_add_event_cb(s_del, delete_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* dl = lv_label_create(s_del);
  lv_label_set_text(dl, "Delete");
  lv_obj_center(dl);

  // Keyboard overlays the bottom half; hidden until the name is tapped.
  s_kb = lv_keyboard_create(scr);
  lv_keyboard_set_textarea(s_kb, s_name);
  lv_obj_add_event_cb(s_kb, kb_done, LV_EVENT_READY, nullptr);
  lv_obj_add_event_cb(s_kb, kb_done, LV_EVENT_CANCEL, nullptr);
  lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
  return s_screen;
}

void preset_edit_load(const char* name, int loF, int hiF, bool stainless,
                      bool canDelete) {
  if (!s_screen) preset_edit_create();
  lv_label_set_text(s_title, canDelete ? "Edit preset" : "New preset");
  lv_textarea_set_text(s_name, name ? name : "");
  s_lo = loF;
  s_hi = hiF;
  refresh_temps();
  if (stainless) lv_obj_add_state(s_stain, LV_STATE_CHECKED);
  else lv_obj_clear_state(s_stain, LV_STATE_CHECKED);
  canDelete ? lv_obj_clear_flag(s_del, LV_OBJ_FLAG_HIDDEN)
            : lv_obj_add_flag(s_del, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
}

}  // namespace ui
