// screen_foods.cpp — see screen_foods.h.
#include "screen_foods.h"

#include <cstdio>

#include "ui/ui_root.h"
#include "core/foodlib.h"

namespace ui {
namespace {
lv_obj_t* s_screen = nullptr;

void food_cb(lv_event_t* e) {
  const int id = (int)(intptr_t)lv_event_get_user_data(e);
  ui::select_food(id);
}
void done_cb(lv_event_t*) { ui::show_home(); }
void recipe_cb(lv_event_t*) { ui::start_recipe(); }
}  // namespace

lv_obj_t* foods_create() {
  if (s_screen) return s_screen;
  lv_obj_t* scr = lv_obj_create(nullptr);
  s_screen = scr;
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "Choose a food");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x8A93A0), 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 12, 8);

  lv_obj_t* done = lv_btn_create(scr);
  lv_obj_set_size(done, 84, 30);
  lv_obj_align(done, LV_ALIGN_TOP_RIGHT, -8, 6);
  lv_obj_add_event_cb(done, done_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* dl = lv_label_create(done);
  lv_label_set_text(dl, "Done");
  lv_obj_center(dl);

  // Recipe entry (M19) — a guided multi-step program.
  lv_obj_t* rec = lv_btn_create(scr);
  lv_obj_set_size(rec, 220, 30);
  lv_obj_align(rec, LV_ALIGN_TOP_LEFT, 140, 6);
  lv_obj_set_style_bg_color(rec, lv_color_hex(0x2E5AAC), 0);
  lv_obj_add_event_cb(rec, recipe_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* rl = lv_label_create(rec);
  lv_label_set_text(rl, LV_SYMBOL_PLAY " Smash Burgers x4");
  lv_obj_center(rl);

  lv_obj_t* list = lv_list_create(scr);
  lv_obj_set_size(list, 464, 238);
  lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, -6);
  lv_obj_set_style_bg_color(list, lv_color_hex(0x101418), 0);

  for (int i = 0; i < foodlib_count(); ++i) {
    const FoodEntry& f = foodlib_entry(i);
    int total = 0;
    for (int s = 0; s < f.sides; ++s) total += f.sideSec[s];
    char label[72];
    std::snprintf(label, sizeof(label), "%s  %s   %d-%d\xC2\xB0" "F  %dm%02ds",
                  f.name, f.variant, f.panTargetF_lo, f.panTargetF_hi,
                  total / 60, total % 60);
    lv_obj_t* btn = lv_list_add_btn(list, nullptr, label);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1A2027), 0);
    lv_obj_set_style_text_color(btn, lv_color_hex(0xF5F5F5), 0);
    lv_obj_add_event_cb(btn, food_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
  }
  return s_screen;
}

}  // namespace ui
