// screen_presets.cpp — see screen_presets.h.
#include "screen_presets.h"

#include <cstdio>

#include "ui/ui_root.h"
#include "ui/list_style.h"
#include "core/presets.h"
#include "core/foodlib.h"
#include "core/favstore.h"

namespace ui {
namespace {

lv_obj_t* s_screen = nullptr;
lv_obj_t* s_grid = nullptr;

void card_cb(lv_event_t* e) {
  const uint8_t id = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
  ui::select_preset(id);
}
void fav_card_cb(lv_event_t* e) {
  ui::select_food((int)(intptr_t)lv_event_get_user_data(e));
}
void program_card_cb(lv_event_t*) { ui::open_programs(); }
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

// (Re)build the card grid: FAVORITE FOODS first (bench 2026-07-12: one-tap
// access to "things they do a lot" — tapping one arms that food's timer +
// target directly), then built-in + custom presets, then an "add" card.
void build_cards() {
  if (!s_grid) return;
  lv_obj_clean(s_grid);
  // 140-wide cards leave the right gutter free for the always-on scrollbar
  // (list_style.h treatment; the grid's content area is 480 - 30 pad_right).
  const int cols = 3, cw = 140, ch = 116, gx = 8, gy = 6;
  const int x0 = 4;
  const int y0 = 4;

  // ONE top-level grid (bench 2026-07-12: "why are there three different
  // classes of these things"): favorites (amber, food timers), the recipe
  // program (blue, multi-step), then plain temp presets (grey). The food
  // picker stays the full catalog; anything used a lot lives HERE.
  int slot0 = 0;
  {
    const int r = slot0 / cols, c = slot0 % cols;
    lv_obj_t* card = lv_btn_create(s_grid);
    lv_obj_set_size(card, cw, ch);
    lv_obj_set_pos(card, x0 + c * (cw + gx), y0 + r * (ch + gy));
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x2E5AAC), 0);
    lv_obj_add_event_cb(card, program_card_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* pl = lv_label_create(card);
    lv_label_set_text(pl, LV_SYMBOL_PLAY "  Recipe\nprograms");
    lv_obj_set_style_text_font(pl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(pl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(pl, LV_ALIGN_CENTER, 0, -8);
    lv_obj_t* sub = lv_label_create(card);
    lv_label_set_text(sub, "built-in + saved");
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(0xB9C8E4), 0);
    lv_obj_align(sub, LV_ALIGN_BOTTOM_MID, 0, -6);
    ++slot0;
  }

  // Favorite-food cards (amber star; star/unstar lives in the food picker).
  const FavStore* fv = ui::favs();
  if (fv && fv->count() > 0) {
    for (int i = 0; i < foodlib_count(); ++i) {
      const FoodEntry& f = foodlib_entry(i);
      if (!fv->has(fav_hash(f.name, f.variant))) continue;
      const int r = slot0 / cols, c = slot0 % cols;
      lv_obj_t* card = lv_btn_create(s_grid);
      lv_obj_set_size(card, cw, ch);
      lv_obj_set_pos(card, x0 + c * (cw + gx), y0 + r * (ch + gy));
      lv_obj_set_style_radius(card, 12, 0);
      lv_obj_set_style_bg_color(card, lv_color_hex(0x24313F), 0);
      lv_obj_set_style_border_width(card, 2, 0);
      lv_obj_set_style_border_color(card, lv_color_hex(0xC08A00), 0);
      lv_obj_add_event_cb(card, fav_card_cb, LV_EVENT_CLICKED,
                          (void*)(intptr_t)i);

      lv_obj_t* star = lv_label_create(card);
      lv_label_set_text(star, "*");
      lv_obj_set_style_text_font(star, &lv_font_montserrat_20, 0);
      lv_obj_set_style_text_color(star, lv_color_hex(0xC08A00), 0);
      lv_obj_align(star, LV_ALIGN_TOP_LEFT, 2, 2);

      // Bounded layout (bench 2026-07-12: "pork chop button overflows"):
      // name wraps within 2 lines, the variant gets its own dot-truncated
      // line with the authoring "[REVIEW]" tag stripped, temps at the bottom.
      lv_obj_t* name = lv_label_create(card);
      lv_label_set_text(name, f.name);
      lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);
      lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
      lv_obj_set_size(name, cw - 14, 34);
      lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_align(name, LV_ALIGN_TOP_MID, 0, 22);

      char vshort[28];
      {
        const char* vs = f.variant;
        const char* cut = std::strstr(vs, "[REVIEW]");
        size_t len = cut ? (size_t)(cut - vs) : std::strlen(vs);
        while (len > 0 && vs[len - 1] == ' ') --len;
        if (len >= sizeof(vshort)) len = sizeof(vshort) - 1;
        std::memcpy(vshort, vs, len);
        vshort[len] = '\0';
      }
      lv_obj_t* v = lv_label_create(card);
      lv_label_set_text(v, vshort);
      lv_obj_set_style_text_font(v, &lv_font_montserrat_14, 0);
      lv_obj_set_style_text_color(v, lv_color_hex(0x9AA3AF), 0);
      lv_label_set_long_mode(v, LV_LABEL_LONG_DOT);
      lv_obj_set_size(v, cw - 14, 18);
      lv_obj_set_style_text_align(v, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_align(v, LV_ALIGN_TOP_MID, 0, 62);

      char rng[20];
      std::snprintf(rng, sizeof(rng), "%d-%d\xC2\xB0" "F", f.panTargetF_lo,
                    f.panTargetF_hi);
      lv_obj_t* t = lv_label_create(card);
      lv_label_set_text(t, rng);
      lv_obj_set_style_text_font(t, &lv_font_montserrat_14, 0);
      lv_obj_set_style_text_color(t, lv_color_hex(0x9AA3AF), 0);
      lv_obj_align(t, LV_ALIGN_BOTTOM_MID, 0, -6);
      ++slot0;
    }
  }

  const int total = presets_total();
  for (int p = 0; p <= total; ++p) {            // last slot = "+ New preset"
    const int slot = slot0 + p;
    const int r = slot / cols, c = slot % cols;
    lv_obj_t* card = lv_btn_create(s_grid);
    lv_obj_set_size(card, cw, ch);
    lv_obj_set_pos(card, x0 + c * (cw + gx), y0 + r * (ch + gy));
    lv_obj_set_style_radius(card, 12, 0);

    if (p == total) {                             // add-new card
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

    const uint8_t id = (uint8_t)p;
    const Preset& pr = preset(id);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1E2530), 0);
    lv_obj_add_event_cb(card, card_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)id);

    lv_obj_t* name = lv_label_create(card);
    lv_label_set_text(name, pr.name);
    lv_obj_set_style_text_font(name, &lv_font_montserrat_20, 0);
    lv_obj_align(name, LV_ALIGN_TOP_MID, 0, 6);

    char rng[32];
    std::snprintf(rng, sizeof(rng), "%d-%d\xC2\xB0" "F", pr.loF, pr.hiF);
    lv_obj_t* range = lv_label_create(card);
    lv_label_set_text(range, rng);
    lv_obj_set_style_text_font(range, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(range, lv_color_hex(0x9AA3AF), 0);
    lv_obj_align(range, LV_ALIGN_CENTER, 0, 6);

    if (pr.stainlessHints) {
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
  apply_scroll_list_style(s_grid);   // the one 90/10 list format (list_style.h)
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
