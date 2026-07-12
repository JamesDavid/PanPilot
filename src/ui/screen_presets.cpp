// screen_presets.cpp — see screen_presets.h.
#include "screen_presets.h"

#include <cstdio>

#include "ui/ui_root.h"
#include "core/presets.h"

namespace ui {
namespace {

lv_obj_t* s_screen = nullptr;
lv_obj_t* s_grid = nullptr;

void card_cb(lv_event_t* e) {
  const uint8_t id = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
  ui::select_preset(id);
}
void edit_cb(lv_event_t* e) {
  const int id = (int)(intptr_t)lv_event_get_user_data(e);
  ui::show_preset_edit(id);
}
void new_cb(lv_event_t*) { ui::show_preset_edit(-1); }
void done_cb(lv_event_t*) { ui::show_home(); }
void learn_cb(lv_event_t*) { ui::show_profiles(); }  // pick/learn the PAN (first-class)
void lastcook_cb(lv_event_t*) { ui::show_lastcook(); }
void foods_cb(lv_event_t*) { ui::cook_a_food(); }   // routes to the open pan's zone
void settings_cb(lv_event_t*) { ui::show_settings(); }

// (Re)build the card grid: built-in + custom presets, then an "add" card.
void build_cards() {
  if (!s_grid) return;
  lv_obj_clean(s_grid);
  const int cols = 3, cw = 148, ch = 116, gx = 8, gy = 6;
  const int x0 = (480 - (cols * cw + (cols - 1) * gx)) / 2;
  const int y0 = 4;
  const int total = presets_total();
  for (int slot = 0; slot <= total; ++slot) {   // last slot = "+ New preset"
    const int r = slot / cols, c = slot % cols;
    lv_obj_t* card = lv_btn_create(s_grid);
    lv_obj_set_size(card, cw, ch);
    lv_obj_set_pos(card, x0 + c * (cw + gx), y0 + r * (ch + gy));
    lv_obj_set_style_radius(card, 12, 0);

    if (slot == total) {                          // add-new card
      lv_obj_set_style_bg_color(card, lv_color_hex(0x243042), 0);
      lv_obj_set_style_border_width(card, 2, 0);
      lv_obj_set_style_border_color(card, lv_color_hex(0x3A4658), 0);
      lv_obj_add_event_cb(card, new_cb, LV_EVENT_CLICKED, nullptr);
      lv_obj_t* plus = lv_label_create(card);
      lv_label_set_text(plus, LV_SYMBOL_PLUS "  New");
      lv_obj_set_style_text_font(plus, &lv_font_montserrat_20, 0);
      lv_obj_set_style_text_color(plus, lv_color_hex(0x9AA3AF), 0);
      lv_obj_center(plus);
      continue;
    }

    const uint8_t id = (uint8_t)slot;
    const Preset& p = preset(id);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1E2530), 0);
    lv_obj_add_event_cb(card, card_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)id);

    lv_obj_t* name = lv_label_create(card);
    lv_label_set_text(name, p.name);
    lv_obj_set_style_text_font(name, &lv_font_montserrat_20, 0);
    lv_obj_align(name, LV_ALIGN_TOP_MID, 0, 6);

    char rng[32];
    std::snprintf(rng, sizeof(rng), "%d-%d\xC2\xB0" "F", p.loF, p.hiF);
    lv_obj_t* range = lv_label_create(card);
    lv_label_set_text(range, rng);
    lv_obj_set_style_text_font(range, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(range, lv_color_hex(0x9AA3AF), 0);
    lv_obj_align(range, LV_ALIGN_CENTER, 0, 6);

    if (p.stainlessHints) {
      lv_obj_t* s = lv_label_create(card);
      lv_label_set_text(s, "stainless");
      lv_obj_set_style_text_font(s, &lv_font_montserrat_14, 0);
      lv_obj_set_style_text_color(s, lv_color_hex(0xC08A00), 0);
      lv_obj_align(s, LV_ALIGN_BOTTOM_MID, 0, -6);
    }

    if (presets_is_custom(id)) {                  // custom -> editable
      lv_obj_t* pen = lv_btn_create(card);
      lv_obj_set_size(pen, 34, 30);
      lv_obj_align(pen, LV_ALIGN_TOP_RIGHT, 2, -4);
      lv_obj_set_style_bg_color(pen, lv_color_hex(0x2A323C), 0);
      lv_obj_add_event_cb(pen, edit_cb, LV_EVENT_CLICKED, (void*)(intptr_t)id);
      lv_obj_t* pl = lv_label_create(pen);
      lv_label_set_text(pl, LV_SYMBOL_EDIT);
      lv_obj_center(pl);
    }
  }
}

}  // namespace

void presets_refresh() { build_cards(); }

lv_obj_t* presets_create() {
  if (s_screen) return s_screen;
  lv_obj_t* scr = lv_obj_create(nullptr);
  s_screen = scr;
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "Presets");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x8A93A0), 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 12, 8);

  // Scrollable grid of cards. Three across; rows past the first two scroll
  // into view by swipe, so custom presets can grow the list without a redesign.
  s_grid = lv_obj_create(scr);
  lv_obj_remove_style_all(s_grid);
  lv_obj_set_size(s_grid, 480, 250);
  lv_obj_align(s_grid, LV_ALIGN_TOP_MID, 0, 34);
  lv_obj_set_scroll_dir(s_grid, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(s_grid, LV_SCROLLBAR_MODE_AUTO);
  build_cards();

  lv_obj_t* done = lv_btn_create(scr);
  lv_obj_set_size(done, 84, 30);
  lv_obj_align(done, LV_ALIGN_TOP_RIGHT, -8, 6);
  lv_obj_add_event_cb(done, done_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* dl = lv_label_create(done);
  lv_label_set_text(dl, "Done");
  lv_obj_center(dl);

  lv_obj_t* foods = lv_btn_create(scr);
  lv_obj_set_size(foods, 150, 30);
  lv_obj_align(foods, LV_ALIGN_TOP_RIGHT, -100, 6);
  lv_obj_set_style_bg_color(foods, lv_color_hex(0x2E7D32), 0);
  lv_obj_add_event_cb(foods, foods_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* fl = lv_label_create(foods);
  lv_label_set_text(fl, "Cook a food " LV_SYMBOL_RIGHT);
  lv_obj_center(fl);

  // Bottom nav: Learn Pan | Settings | Last Cook.
  lv_obj_t* learn = lv_btn_create(scr);
  lv_obj_set_size(learn, 150, 28);
  lv_obj_align(learn, LV_ALIGN_BOTTOM_MID, -158, -2);
  lv_obj_set_style_bg_color(learn, lv_color_hex(0x2A323C), 0);
  lv_obj_add_event_cb(learn, learn_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* ll = lv_label_create(learn);
  lv_label_set_text(ll, "Pans " LV_SYMBOL_RIGHT);   // select the pan for the cook
  lv_obj_center(ll);

  lv_obj_t* set = lv_btn_create(scr);
  lv_obj_set_size(set, 150, 28);
  lv_obj_align(set, LV_ALIGN_BOTTOM_MID, 0, -2);
  lv_obj_set_style_bg_color(set, lv_color_hex(0x2A323C), 0);
  lv_obj_add_event_cb(set, settings_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* sl = lv_label_create(set);
  lv_label_set_text(sl, LV_SYMBOL_SETTINGS " Settings");
  lv_obj_center(sl);

  lv_obj_t* lc = lv_btn_create(scr);
  lv_obj_set_size(lc, 150, 28);
  lv_obj_align(lc, LV_ALIGN_BOTTOM_MID, 158, -2);
  lv_obj_set_style_bg_color(lc, lv_color_hex(0x2A323C), 0);
  lv_obj_add_event_cb(lc, lastcook_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* lcl = lv_label_create(lc);
  lv_label_set_text(lcl, "Last Cook " LV_SYMBOL_RIGHT);
  lv_obj_center(lcl);
  return s_screen;
}

}  // namespace ui
