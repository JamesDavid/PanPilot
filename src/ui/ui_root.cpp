// ui_root.cpp — see ui_root.h.
#include "ui_root.h"

#include <lvgl.h>

#include "ui/screen_home.h"
#include "ui/screen_thermal.h"
#include "ui/screen_presets.h"
#include "ui/screen_learn.h"
#include "ui/screen_lastcook.h"
#include "ui/screen_foods.h"

namespace ui {
namespace {
lv_obj_t* s_home = nullptr;
lv_obj_t* s_thermal = nullptr;
lv_obj_t* s_presets = nullptr;
lv_obj_t* s_idle = nullptr;
lv_obj_t* s_learn = nullptr;
lv_obj_t* s_lastcook = nullptr;
lv_obj_t* s_foods = nullptr;
bool s_useF = true;
UnitChangeCb s_unitCb = nullptr;
TargetDeltaCb s_targetCb = nullptr;
PresetCb s_presetCb = nullptr;
LearnCb s_learnCb = nullptr;
FoodCb s_foodCb = nullptr;
enum Active { HOME, THERMAL, PRESETS, IDLE_SCREEN, LEARN, LASTCOOK, FOODS } s_active = HOME;
}  // namespace

void root_init(bool useF, UnitChangeCb onUnit, TargetDeltaCb onTargetDelta,
               PresetCb onPreset, LearnCb onLearn, FoodCb onFood) {
  s_useF = useF;
  s_unitCb = onUnit;
  s_targetCb = onTargetDelta;
  s_presetCb = onPreset;
  s_learnCb = onLearn;
  s_foodCb = onFood;
  s_home = home_create();
  s_thermal = thermal_create();
  s_presets = presets_create();
  s_learn = learn_create();
  s_lastcook = lastcook_create();
  s_foods = foods_create();
  lv_scr_load(s_home);
  s_active = HOME;
}

void show_home() { if (s_home) { lv_scr_load(s_home); s_active = HOME; } }
void show_thermal() { if (s_thermal) { lv_scr_load(s_thermal); s_active = THERMAL; } }
void show_presets() { if (s_presets) { lv_scr_load(s_presets); s_active = PRESETS; } }

void show_idle() {
  if (!s_idle) {
    s_idle = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(s_idle, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_t* t = lv_label_create(s_idle);
    lv_label_set_text(t, "PanPilot");
    lv_obj_set_style_text_font(t, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(t, lv_color_hex(0x6A7480), 0);
    lv_obj_align(t, LV_ALIGN_CENTER, 0, -14);
    lv_obj_t* m = lv_label_create(s_idle);
    lv_label_set_text(m, "monitoring - tap or heat a pan");
    lv_obj_set_style_text_font(m, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(m, lv_color_hex(0x4A525C), 0);
    lv_obj_align(m, LV_ALIGN_CENTER, 0, 20);
  }
  lv_scr_load(s_idle);
  s_active = IDLE_SCREEN;
}

void show_learn() { if (s_learn) { lv_scr_load(s_learn); s_active = LEARN; } }
void show_lastcook() { if (s_lastcook) { lv_scr_load(s_lastcook); s_active = LASTCOOK; } }
void show_foods() { if (s_foods) { lv_scr_load(s_foods); s_active = FOODS; } }

void toggle_unit() { s_useF = !s_useF; if (s_unitCb) s_unitCb(s_useF); }
bool unit_useF() { return s_useF; }
void target_adjust(int deltaF) { if (s_targetCb) s_targetCb(deltaF); }
void select_preset(uint8_t id) { if (s_presetCb) s_presetCb(id); show_home(); }
void learn_cmd(uint8_t cmd) { if (s_learnCb) s_learnCb(cmd); }
void select_food(int id) { if (s_foodCb) s_foodCb(id); show_home(); }

void root_update(const ThermalFrame& f, const PanReading& r, const UiState& s) {
  if (s_active == HOME) home_update(s, s_useF);
  else if (s_active == THERMAL) thermal_update(f, r, s_useF);
  else if (s_active == LEARN) learn_update(s);
  // presets / idle screens are static; nothing to update per tick
}

}  // namespace ui
