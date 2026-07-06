// ui_root.cpp — see ui_root.h.
#include "ui_root.h"

#include <lvgl.h>

#include "ui/screen_home.h"
#include "ui/screen_thermal.h"
#include "ui/screen_presets.h"
#include "ui/screen_learn.h"
#include "ui/screen_lastcook.h"
#include "ui/screen_foods.h"
#include "ui/screen_settings.h"
#include "core/settings.h"

namespace ui {
namespace {
lv_obj_t* s_home = nullptr;
lv_obj_t* s_thermal = nullptr;
lv_obj_t* s_presets = nullptr;
lv_obj_t* s_idle = nullptr;
lv_obj_t* s_learn = nullptr;
lv_obj_t* s_lastcook = nullptr;
lv_obj_t* s_foods = nullptr;
lv_obj_t* s_settings = nullptr;
bool s_useF = true;
bool s_muted = false;        // cached from the per-tick snapshot for Settings
uint8_t s_bright = 2;
UnitChangeCb s_unitCb = nullptr;
TargetDeltaCb s_targetCb = nullptr;
PresetCb s_presetCb = nullptr;
PresetCb s_preset2Cb = nullptr;
LearnCb s_learnCb = nullptr;
FoodCb s_foodCb = nullptr;
RecipeCb s_recipeCb = nullptr;
MuteCb s_muteCb = nullptr;
BrightnessCb s_brightCb = nullptr;
uint8_t s_presetZone = 0;   // which zone the preset picker edits (0/1)
enum Active { HOME, THERMAL, PRESETS, IDLE_SCREEN, LEARN, LASTCOOK, FOODS, SETTINGS } s_active = HOME;

void settings_refresh() { settings_update(s_useF, s_muted, s_bright); }
}  // namespace

void root_init(bool useF, UnitChangeCb onUnit, TargetDeltaCb onTargetDelta,
               PresetCb onPreset, LearnCb onLearn, FoodCb onFood, PresetCb onPreset2,
               RecipeCb onRecipe) {
  s_useF = useF;
  s_unitCb = onUnit;
  s_targetCb = onTargetDelta;
  s_presetCb = onPreset;
  s_preset2Cb = onPreset2;
  s_learnCb = onLearn;
  s_foodCb = onFood;
  s_recipeCb = onRecipe;
  s_home = home_create();
  s_thermal = thermal_create();
  s_presets = presets_create();
  s_learn = learn_create();
  s_lastcook = lastcook_create();
  s_foods = foods_create();
  s_settings = settings_create();
  lv_scr_load(s_home);
  s_active = HOME;
}

void set_settings_cbs(MuteCb onMute, BrightnessCb onBrightness) {
  s_muteCb = onMute;
  s_brightCb = onBrightness;
}

void show_home() { if (s_home) { lv_scr_load(s_home); s_active = HOME; } }
void show_thermal() { if (s_thermal) { lv_scr_load(s_thermal); s_active = THERMAL; } }

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
void show_presets() { if (s_presets) { s_presetZone = 0; lv_scr_load(s_presets); s_active = PRESETS; } }
void show_presets_zone2() { if (s_presets) { s_presetZone = 1; lv_scr_load(s_presets); s_active = PRESETS; } }
void show_settings() {
  if (!s_settings) s_settings = settings_create();
  settings_refresh();
  lv_scr_load(s_settings);
  s_active = SETTINGS;
}

void toggle_unit() { s_useF = !s_useF; if (s_unitCb) s_unitCb(s_useF); }

void settings_toggle_unit() { toggle_unit(); settings_refresh(); }
void settings_toggle_mute() {
  const bool nm = !s_muted;
  if (s_muteCb) s_muteCb(nm);
  s_muted = nm;
  settings_refresh();
}
void settings_cycle_brightness() {
  const uint8_t nb = panpilot::brightness_cycle(s_bright);
  if (s_brightCb) s_brightCb(nb);
  s_bright = nb;
  settings_refresh();
}
bool unit_useF() { return s_useF; }
void target_adjust(int deltaF) { if (s_targetCb) s_targetCb(deltaF); }
void select_preset(uint8_t id) {
  if (s_presetZone == 1 && s_preset2Cb) s_preset2Cb(id);
  else if (s_presetCb) s_presetCb(id);
  s_presetZone = 0;
  show_home();
}
void learn_cmd(uint8_t cmd) { if (s_learnCb) s_learnCb(cmd); }
void select_food(int id) { if (s_foodCb) s_foodCb(id); show_home(); }
void recipe_cmd(uint8_t cmd) { if (s_recipeCb) s_recipeCb(cmd); }
void start_recipe() { if (s_recipeCb) s_recipeCb(0); show_home(); }

void root_update(const ThermalFrame& f, const PanReading& r, const UiState& s) {
  s_muted = s.muted;                 // keep Settings' cache in sync with device truth
  s_bright = s.brightnessLevel;
  if (s_active == HOME) home_update(s, s_useF);
  else if (s_active == THERMAL) thermal_update(f, r, s_useF);
  else if (s_active == LEARN) learn_update(s);
  else if (s_active == SETTINGS) settings_refresh();
  // presets / idle screens are static; nothing to update per tick
}

}  // namespace ui
