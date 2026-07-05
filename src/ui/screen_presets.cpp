// screen_presets.cpp — see screen_presets.h.
#include "screen_presets.h"

#include <cstdio>

#include "ui/ui_root.h"
#include "core/presets.h"

namespace ui {
namespace {

lv_obj_t* s_screen = nullptr;

void card_cb(lv_event_t* e) {
  const uint8_t id = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
  ui::select_preset(id);
}
void done_cb(lv_event_t*) { ui::show_home(); }
void learn_cb(lv_event_t*) { ui::show_learn(); }

}  // namespace

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

  // 3x2 grid of cards
  const int cols = 3, cw = 148, ch = 116, gx = 8, gy = 6;
  const int x0 = (480 - (cols * cw + (cols - 1) * gx)) / 2;
  const int y0 = 36;
  for (uint8_t id = 0; id < PRESET_COUNT; ++id) {
    const Preset& p = preset(id);
    const int r = id / cols, c = id % cols;
    lv_obj_t* card = lv_btn_create(scr);
    lv_obj_set_size(card, cw, ch);
    lv_obj_set_pos(card, x0 + c * (cw + gx), y0 + r * (ch + gy));
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1E2530), 0);
    lv_obj_set_style_radius(card, 12, 0);
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
  }

  lv_obj_t* done = lv_btn_create(scr);
  lv_obj_set_size(done, 84, 30);
  lv_obj_align(done, LV_ALIGN_TOP_RIGHT, -8, 6);
  lv_obj_add_event_cb(done, done_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* dl = lv_label_create(done);
  lv_label_set_text(dl, "Done");
  lv_obj_center(dl);

  lv_obj_t* learn = lv_btn_create(scr);
  lv_obj_set_size(learn, 200, 28);
  lv_obj_align(learn, LV_ALIGN_BOTTOM_MID, 0, -2);
  lv_obj_set_style_bg_color(learn, lv_color_hex(0x2A323C), 0);
  lv_obj_add_event_cb(learn, learn_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* ll = lv_label_create(learn);
  lv_label_set_text(ll, "Learn Pan Mode " LV_SYMBOL_RIGHT);
  lv_obj_center(ll);
  return s_screen;
}

}  // namespace ui
