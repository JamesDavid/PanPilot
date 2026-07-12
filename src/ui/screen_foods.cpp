// screen_foods.cpp — see screen_foods.h.
#include "screen_foods.h"

#include <cstdio>

#include "ui/ui_root.h"
#include "ui/list_style.h"
#include "core/foodlib.h"
#include "core/favstore.h"

namespace ui {
namespace {
lv_obj_t* s_screen = nullptr;
lv_obj_t* s_list = nullptr;

void food_cb(lv_event_t* e) {
  const int id = (int)(intptr_t)lv_event_get_user_data(e);
  ui::select_food(id);
}
void star_cb(lv_event_t* e) {
  ui::food_fav((int)(intptr_t)lv_event_get_user_data(e));
}
void done_cb(lv_event_t*) { ui::show_home(); }
void recipe_cb(lv_event_t*) { ui::open_programs(); }

bool is_fav(int i) {
  const FavStore* f = ui::favs();
  if (!f) return false;
  const FoodEntry& e = foodlib_entry(i);
  return f->has(fav_hash(e.name, e.variant));
}

void add_food_row(int i, bool fav) {
  const FoodEntry& f = foodlib_entry(i);
  int total = 0;
  for (int s = 0; s < f.sides; ++s) total += f.sideSec[s];

  lv_obj_t* row = lv_btn_create(s_list);
  lv_obj_set_width(row, lv_pct(100));   // of the content area; the list's
  lv_obj_set_height(row, 48);           // pad_right reserves the scrollbar lane
  lv_obj_set_style_bg_color(row, lv_color_hex(fav ? 0x24313F : 0x1A2027), 0);
  lv_obj_set_style_radius(row, 8, 0);
  lv_obj_add_event_cb(row, food_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);

  // One recolored two-line label per row keeps the object count low: name
  // (line 1), "variant · temp · time" (line 2, grey).
  char txt[160];
  std::snprintf(txt, sizeof(txt),
                "%s\n#8a93a0 %s   |   %d-%d\xC2\xB0" "F   |   %d:%02d#",
                f.name, f.variant, f.panTargetF_lo, f.panTargetF_hi,
                total / 60, total % 60);
  lv_obj_t* lab = lv_label_create(row);
  lv_label_set_recolor(lab, true);
  lv_label_set_text(lab, txt);
  lv_obj_set_style_text_font(lab, &lv_font_montserrat_14, 0);
  lv_obj_align(lab, LV_ALIGN_LEFT_MID, 4, 0);

  // Star chip: favorites surface to the top of this list AND onto the preset
  // picker as one-tap cards (bench 2026-07-12 "things they do a lot").
  lv_obj_t* star = lv_btn_create(row);
  lv_obj_set_size(star, 44, 40);
  lv_obj_align(star, LV_ALIGN_RIGHT_MID, -2, 0);
  lv_obj_set_style_bg_color(star, lv_color_hex(fav ? 0xC08A00 : 0x2A323C), 0);
  lv_obj_add_event_cb(star, star_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
  lv_obj_t* sl = lv_label_create(star);
  lv_label_set_text(sl, "*");
  lv_obj_set_style_text_font(sl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(sl, lv_color_hex(fav ? 0x101418 : 0x8A93A0), 0);
  lv_obj_center(sl);
}
}  // namespace

void foods_refresh() {
  if (!s_list) return;
  lv_obj_clean(s_list);

  // Favorites first...
  for (int i = 0; i < foodlib_count(); ++i)
    if (is_fav(i)) add_food_row(i, true);

  // ...then the built-in recipe PROGRAM (M19) — visually distinct (blue) but
  // scrolling with everything else (bench feedback: no pinned band). Foods
  // are single-food timers; this row runs the multi-step sequencer.
  {
    lv_obj_t* row = lv_btn_create(s_list);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 48);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x2E5AAC), 0);
    lv_obj_set_style_radius(row, 8, 0);
    lv_obj_add_event_cb(row, recipe_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* rl = lv_label_create(row);
    lv_label_set_text(rl, LV_SYMBOL_PLAY "  Recipe programs (built-in + saved)");
    lv_obj_set_style_text_font(rl, &lv_font_montserrat_14, 0);
    lv_obj_align(rl, LV_ALIGN_LEFT_MID, 4, 0);
  }

  // ...then everything not starred.
  for (int i = 0; i < foodlib_count(); ++i)
    if (!is_fav(i)) add_food_row(i, false);
}

lv_obj_t* foods_create() {
  if (s_screen) return s_screen;
  lv_obj_t* scr = lv_obj_create(nullptr);
  s_screen = scr;
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  // Header row: title (left) + Done (right).
  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "Choose a food");
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

  // Scrollable food list — two-line rows so nothing truncates.
  s_list = lv_list_create(scr);
  lv_obj_set_size(s_list, 468, 272);
  lv_obj_align(s_list, LV_ALIGN_BOTTOM_MID, 0, -6);
  lv_obj_set_style_bg_color(s_list, lv_color_hex(0x101418), 0);
  lv_obj_set_style_pad_row(s_list, 4, 0);
  apply_scroll_list_style(s_list);   // the one 90/10 list format (list_style.h)

  foods_refresh();
  return s_screen;
}

}  // namespace ui
