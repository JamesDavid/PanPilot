// screen_onboarding.cpp — see screen_onboarding.h.
#include "screen_onboarding.h"

#include "ui/ui_root.h"

namespace ui {
namespace {
constexpr int STEPS = 4;
lv_obj_t* s_screen = nullptr;
lv_obj_t* s_title = nullptr;
lv_obj_t* s_body = nullptr;
lv_obj_t* s_uf = nullptr;         // °F button
lv_obj_t* s_uc = nullptr;         // °C button
lv_obj_t* s_back = nullptr;
lv_obj_t* s_next_lbl = nullptr;
lv_obj_t* s_dots[STEPS] = {nullptr};
int s_step = 0;
bool s_useF = true;

struct Page { const char* title; const char* body; };
const Page PAGES[STEPS] = {
  {"Welcome to PanPilot",
   "Your hands-free pan-temperature coach. This takes about 30 seconds."},
  {"Temperature units",
   "How should I show temperatures?"},
  {"Mount & aim",
   "Place PanPilot 12-18 in above the burner with the sensor pointed at the "
   "pan center. You can check your aim any time in the thermal view."},
  {"You're all set",
   "Pick a preset or a food, then heat your pan. I'll watch the temperature "
   "and cue you when to flip, turn down, or pull."},
};

void highlight_units() {
  lv_obj_set_style_bg_color(s_uf, lv_color_hex(s_useF ? 0x2E5AAC : 0x2A323C), 0);
  lv_obj_set_style_bg_color(s_uc, lv_color_hex(s_useF ? 0x2A323C : 0x2E5AAC), 0);
}

void render() {
  lv_label_set_text(s_title, PAGES[s_step].title);
  lv_label_set_text(s_body, PAGES[s_step].body);
  const bool units = (s_step == 1);
  units ? lv_obj_clear_flag(s_uf, LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(s_uf, LV_OBJ_FLAG_HIDDEN);
  units ? lv_obj_clear_flag(s_uc, LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(s_uc, LV_OBJ_FLAG_HIDDEN);
  if (units) highlight_units();
  s_step == 0 ? lv_obj_add_flag(s_back, LV_OBJ_FLAG_HIDDEN)
              : lv_obj_clear_flag(s_back, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(s_next_lbl,
                    s_step == STEPS - 1 ? "Start cooking" : "Next " LV_SYMBOL_RIGHT);
  for (int i = 0; i < STEPS; ++i)
    lv_obj_set_style_bg_color(s_dots[i],
        lv_color_hex(i == s_step ? 0xF5F5F5 : 0x3A4048), 0);
}

void set_unit(bool useF) { s_useF = useF; ui::set_display_unit(useF); highlight_units(); }
void uf_cb(lv_event_t*) { set_unit(true); }
void uc_cb(lv_event_t*) { set_unit(false); }
void back_cb(lv_event_t*) { if (s_step > 0) { --s_step; render(); } }
void next_cb(lv_event_t*) {
  if (s_step < STEPS - 1) { ++s_step; render(); }
  else ui::onboarding_finish();
}
}  // namespace

lv_obj_t* onboarding_create() {
  if (s_screen) return s_screen;
  lv_obj_t* scr = lv_obj_create(nullptr);
  s_screen = scr;
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  s_title = lv_label_create(scr);
  lv_obj_set_style_text_font(s_title, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(s_title, lv_color_hex(0xF5F5F5), 0);
  lv_obj_align(s_title, LV_ALIGN_TOP_MID, 0, 40);

  s_body = lv_label_create(scr);
  lv_label_set_long_mode(s_body, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(s_body, 420);
  lv_obj_set_style_text_align(s_body, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(s_body, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(s_body, lv_color_hex(0xB7BEC8), 0);
  lv_obj_align(s_body, LV_ALIGN_TOP_MID, 0, 96);

  s_uf = lv_btn_create(scr);
  lv_obj_set_size(s_uf, 120, 56);
  lv_obj_align(s_uf, LV_ALIGN_CENTER, -72, 40);
  lv_obj_add_event_cb(s_uf, uf_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* ufl = lv_label_create(s_uf);
  lv_label_set_text(ufl, "\xC2\xB0" "F");
  lv_obj_set_style_text_font(ufl, &lv_font_montserrat_28, 0);
  lv_obj_center(ufl);

  s_uc = lv_btn_create(scr);
  lv_obj_set_size(s_uc, 120, 56);
  lv_obj_align(s_uc, LV_ALIGN_CENTER, 72, 40);
  lv_obj_add_event_cb(s_uc, uc_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* ucl = lv_label_create(s_uc);
  lv_label_set_text(ucl, "\xC2\xB0" "C");
  lv_obj_set_style_text_font(ucl, &lv_font_montserrat_28, 0);
  lv_obj_center(ucl);

  // step dots
  const int dw = 12, gap = 10;
  const int x0 = -((STEPS - 1) * (dw + gap)) / 2;
  for (int i = 0; i < STEPS; ++i) {
    s_dots[i] = lv_obj_create(scr);
    lv_obj_remove_style_all(s_dots[i]);
    lv_obj_set_size(s_dots[i], dw, dw);
    lv_obj_set_style_radius(s_dots[i], dw / 2, 0);
    lv_obj_set_style_bg_opa(s_dots[i], LV_OPA_COVER, 0);
    lv_obj_align(s_dots[i], LV_ALIGN_BOTTOM_MID, x0 + i * (dw + gap), -70);
  }

  s_back = lv_btn_create(scr);
  lv_obj_set_size(s_back, 120, 44);
  lv_obj_align(s_back, LV_ALIGN_BOTTOM_LEFT, 12, -10);
  lv_obj_set_style_bg_color(s_back, lv_color_hex(0x2A323C), 0);
  lv_obj_add_event_cb(s_back, back_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* bl = lv_label_create(s_back);
  lv_label_set_text(bl, LV_SYMBOL_LEFT " Back");
  lv_obj_set_style_text_font(bl, &lv_font_montserrat_20, 0);
  lv_obj_center(bl);

  lv_obj_t* next = lv_btn_create(scr);
  lv_obj_set_size(next, 180, 44);
  lv_obj_align(next, LV_ALIGN_BOTTOM_RIGHT, -12, -10);
  lv_obj_set_style_bg_color(next, lv_color_hex(0x2E7D32), 0);
  lv_obj_add_event_cb(next, next_cb, LV_EVENT_CLICKED, nullptr);
  s_next_lbl = lv_label_create(next);
  lv_obj_set_style_text_font(s_next_lbl, &lv_font_montserrat_20, 0);
  lv_obj_center(s_next_lbl);

  render();
  return s_screen;
}

void onboarding_reset(bool useF) {
  if (!s_screen) onboarding_create();
  s_useF = useF;
  s_step = 0;
  render();
}

}  // namespace ui
