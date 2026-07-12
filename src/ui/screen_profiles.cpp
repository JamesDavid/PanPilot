// screen_profiles.cpp — see screen_profiles.h.
#include "screen_profiles.h"

#include <cstdio>

#include "ui/ui_root.h"
#include "ui/list_style.h"
#include "core/profilestore.h"

namespace ui {
namespace {
lv_obj_t* s_screen = nullptr;
lv_obj_t* s_list = nullptr;
lv_obj_t* s_empty = nullptr;
lv_obj_t* s_map = nullptr;   // "Map burner" — hidden until a pan exists

// Rename overlay (bench 2026-07-12 "can we name the pans?"): textarea +
// keyboard over this screen; OK routes through ui::profile_rename.
lv_obj_t* s_ren = nullptr;
lv_obj_t* s_ren_ta = nullptr;
int s_renIdx = -1;

void done_cb(lv_event_t*) { ui::show_home(); }
void learn_cb(lv_event_t*) { ui::show_learn(); }
void map_cb(lv_event_t*) { ui::show_burnermap(); }   // calibrates the ACTIVE pan
void act_cb(lv_event_t* e) {
  ui::profile_cmd(0, (int)(intptr_t)lv_event_get_user_data(e));   // activate
}
void del_cb(lv_event_t* e) {
  ui::profile_cmd(1, (int)(intptr_t)lv_event_get_user_data(e));   // delete
}
void stain_cb(lv_event_t* e) {
  ui::profile_cmd(2, (int)(intptr_t)lv_event_get_user_data(e));   // toggle SS
}

void ren_ok_cb(lv_event_t*) {
  if (s_renIdx >= 0) ui::profile_rename(s_renIdx, lv_textarea_get_text(s_ren_ta));
  s_renIdx = -1;
  lv_obj_add_flag(s_ren, LV_OBJ_FLAG_HIDDEN);
}
void ren_cancel_cb(lv_event_t*) {
  s_renIdx = -1;
  lv_obj_add_flag(s_ren, LV_OBJ_FLAG_HIDDEN);
}

void ren_build() {
  if (s_ren) return;
  s_ren = lv_obj_create(s_screen);
  lv_obj_set_size(s_ren, 480, 320);
  lv_obj_center(s_ren);
  lv_obj_set_style_bg_color(s_ren, lv_color_hex(0x101418), 0);
  lv_obj_set_style_bg_opa(s_ren, LV_OPA_COVER, 0);
  lv_obj_clear_flag(s_ren, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* t = lv_label_create(s_ren);
  lv_label_set_text(t, "Pan name");
  lv_obj_set_style_text_font(t, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(t, lv_color_hex(0x8A93A0), 0);
  lv_obj_align(t, LV_ALIGN_TOP_LEFT, 8, 4);

  s_ren_ta = lv_textarea_create(s_ren);
  lv_textarea_set_one_line(s_ren_ta, true);
  lv_textarea_set_max_length(s_ren_ta, 15);   // PanProfile.name[16]
  lv_obj_set_size(s_ren_ta, 250, 44);
  lv_obj_align(s_ren_ta, LV_ALIGN_TOP_LEFT, 100, 34);

  lv_obj_t* ok = lv_btn_create(s_ren);
  lv_obj_set_size(ok, 60, 44);
  lv_obj_align(ok, LV_ALIGN_TOP_LEFT, 360, 34);
  lv_obj_set_style_bg_color(ok, lv_color_hex(0x2E7D32), 0);
  lv_obj_add_event_cb(ok, ren_ok_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* okl = lv_label_create(ok);
  lv_label_set_text(okl, LV_SYMBOL_OK);
  lv_obj_center(okl);

  lv_obj_t* no = lv_btn_create(s_ren);
  lv_obj_set_size(no, 44, 44);
  lv_obj_align(no, LV_ALIGN_TOP_RIGHT, -8, 34);
  lv_obj_set_style_bg_color(no, lv_color_hex(0x2A323C), 0);
  lv_obj_add_event_cb(no, ren_cancel_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* nol = lv_label_create(no);
  lv_label_set_text(nol, LV_SYMBOL_CLOSE);
  lv_obj_center(nol);

  lv_obj_t* kb = lv_keyboard_create(s_ren);
  lv_obj_set_size(kb, 464, 200);
  lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, -4);
  lv_keyboard_set_textarea(kb, s_ren_ta);

  lv_obj_add_flag(s_ren, LV_OBJ_FLAG_HIDDEN);
}

void ren_cb(lv_event_t* e) {
  const int i = (int)(intptr_t)lv_event_get_user_data(e);
  ren_build();
  s_renIdx = i;
  lv_textarea_set_text(s_ren_ta, ui::profile_name(i));
  lv_obj_clear_flag(s_ren, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(s_ren);
}
}  // namespace

lv_obj_t* profiles_create() {
  if (s_screen) return s_screen;
  lv_obj_t* scr = lv_obj_create(nullptr);
  s_screen = scr;
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "My Pans");
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

  lv_obj_t* learn = lv_btn_create(scr);
  lv_obj_set_size(learn, 150, 30);
  lv_obj_align(learn, LV_ALIGN_TOP_RIGHT, -100, 6);
  lv_obj_set_style_bg_color(learn, lv_color_hex(0x2E7D32), 0);
  lv_obj_add_event_cb(learn, learn_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* llb = lv_label_create(learn);
  lv_label_set_text(llb, "Learn a pan " LV_SYMBOL_RIGHT);
  lv_obj_center(llb);

  s_map = lv_btn_create(scr);
  lv_obj_set_size(s_map, 118, 30);
  lv_obj_align(s_map, LV_ALIGN_TOP_RIGHT, -258, 6);
  lv_obj_set_style_bg_color(s_map, lv_color_hex(0xE07000), 0);
  lv_obj_add_event_cb(s_map, map_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* ml = lv_label_create(s_map);
  lv_label_set_text(ml, "Map burner");
  lv_obj_center(ml);
  lv_obj_add_flag(s_map, LV_OBJ_FLAG_HIDDEN);

  s_list = lv_list_create(scr);
  lv_obj_set_size(s_list, 468, 250);
  lv_obj_align(s_list, LV_ALIGN_BOTTOM_MID, 0, -8);
  lv_obj_set_style_bg_color(s_list, lv_color_hex(0x101418), 0);
  lv_obj_set_style_pad_row(s_list, 4, 0);
  apply_scroll_list_style(s_list);   // the one 90/10 list format (list_style.h)

  s_empty = lv_label_create(scr);
  lv_label_set_text(s_empty, "No saved pans yet.\nTap \"Learn a pan\" to teach PanPilot one.");
  lv_obj_set_style_text_align(s_empty, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(s_empty, lv_color_hex(0x8A93A0), 0);
  lv_obj_center(s_empty);
  lv_obj_add_flag(s_empty, LV_OBJ_FLAG_HIDDEN);
  return s_screen;
}

void profiles_update(const ProfileStore& ps) {
  if (!s_screen) profiles_create();
  lv_obj_clean(s_list);

  const int n = ps.count();
  (n == 0) ? lv_obj_clear_flag(s_empty, LV_OBJ_FLAG_HIDDEN)
           : lv_obj_add_flag(s_empty, LV_OBJ_FLAG_HIDDEN);
  // Map burner works on the active pan, so it needs a pan to exist.
  (n == 0) ? lv_obj_add_flag(s_map, LV_OBJ_FLAG_HIDDEN)
           : lv_obj_clear_flag(s_map, LV_OBJ_FLAG_HIDDEN);

  for (int i = 0; i < n; ++i) {
    const bool active = (i == ps.active());
    const PanProfile& p = ps.at(i);

    lv_obj_t* row = lv_btn_create(s_list);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 52);
    lv_obj_set_style_bg_color(row, lv_color_hex(active ? 0x2E5AAC : 0x1A2027), 0);
    lv_obj_set_style_radius(row, 8, 0);
    lv_obj_add_event_cb(row, act_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);

    // Two-line label (name + grey lag) frees the right side for the chips.
    lv_obj_t* nm = lv_label_create(row);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s%s\n#8a93a0 lag %.1f min#",
                  active ? LV_SYMBOL_OK "  " : "", p.name, p.lagMinutes);
    lv_label_set_recolor(nm, true);
    lv_label_set_text(nm, buf);
    lv_obj_set_style_text_font(nm, &lv_font_montserrat_14, 0);
    lv_obj_align(nm, LV_ALIGN_LEFT_MID, 8, 0);

    // Rename pencil (bench: "can we name the pans?").
    lv_obj_t* ren = lv_btn_create(row);
    lv_obj_set_size(ren, 44, 40);
    lv_obj_align(ren, LV_ALIGN_RIGHT_MID, -100, 0);
    lv_obj_set_style_bg_color(ren, lv_color_hex(0x2A323C), 0);
    lv_obj_add_event_cb(ren, ren_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
    lv_obj_t* rl = lv_label_create(ren);
    lv_label_set_text(rl, LV_SYMBOL_EDIT);
    lv_obj_center(rl);

    // Pan-material chip: amber "SS" when stainless; tap to toggle. The pan
    // carries its material — selecting a stainless pan turns on the stainless
    // guidance behavior for the whole cook.
    lv_obj_t* ss = lv_btn_create(row);
    lv_obj_set_size(ss, 44, 40);
    lv_obj_align(ss, LV_ALIGN_RIGHT_MID, -52, 0);
    lv_obj_set_style_bg_color(ss, lv_color_hex(p.stainless ? 0xC08A00 : 0x2A323C), 0);
    lv_obj_add_event_cb(ss, stain_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
    lv_obj_t* sl = lv_label_create(ss);
    lv_label_set_text(sl, "SS");
    lv_obj_set_style_text_font(sl, &lv_font_montserrat_14, 0);
    lv_obj_center(sl);

    lv_obj_t* del = lv_btn_create(row);
    lv_obj_set_size(del, 40, 40);
    lv_obj_align(del, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_obj_set_style_bg_color(del, lv_color_hex(0xC62828), 0);
    lv_obj_add_event_cb(del, del_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
    lv_obj_t* dx = lv_label_create(del);
    lv_label_set_text(dx, LV_SYMBOL_TRASH);
    lv_obj_center(dx);
  }
}

}  // namespace ui
