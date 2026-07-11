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
#include "ui/screen_preset_edit.h"
#include "ui/screen_assist.h"
#include "ui/screen_onboarding.h"
#include "ui/screen_autotune.h"
#include "ui/screen_profiles.h"
#include "core/profilestore.h"
#include "core/settings.h"
#include "core/timezones.h"
#include "core/presets.h"

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
lv_obj_t* s_preset_edit = nullptr;
lv_obj_t* s_assist = nullptr;
lv_obj_t* s_onboarding = nullptr;
lv_obj_t* s_autotune = nullptr;
int s_editId = -1;                 // preset id being edited (-1 = new)
uint8_t s_editReturnZone = 0;      // which pan's picker the editor came from
bool s_assistAvail = false;        // actuator configured (from the snapshot)
const char* s_actuatorName = "";
bool s_useF = true;
bool s_muted = false;        // cached from the per-tick snapshot for Settings
uint8_t s_bright = 2;
uint8_t s_tz = 0;
bool s_stainless = false;
UnitChangeCb s_unitCb = nullptr;
TargetDeltaCb s_targetCb = nullptr;
PresetCb s_presetCb = nullptr;
PresetCb s_preset2Cb = nullptr;
LearnCb s_learnCb = nullptr;
FoodCb s_foodCb = nullptr;
FoodCb s_food2Cb = nullptr;
uint8_t s_foodZone = 0;      // which pan the food picker arms (0/1)
RecipeCb s_recipeCb = nullptr;
MuteCb s_muteCb = nullptr;
BrightnessCb s_brightCb = nullptr;
TimezoneCb s_tzCb = nullptr;
StainlessCb s_stainlessCb = nullptr;
FeedbackCb s_feedbackCb = nullptr;
PresetSaveCb s_presetSaveCb = nullptr;
PresetDeleteCb s_presetDelCb = nullptr;
AssistCb s_assistCb = nullptr;
OnboardingDoneCb s_onboardCb = nullptr;
AutotuneCb s_autotuneCb = nullptr;
RoiCb s_roiCb = nullptr;
const ProfileStore* s_profiles = nullptr;
ProfileCb s_profileCb = nullptr;
uint8_t s_presetZone = 0;   // which zone the preset picker edits (0/1)
enum Active { HOME, THERMAL, PRESETS, IDLE_SCREEN, LEARN, LASTCOOK, FOODS,
              SETTINGS, PRESET_EDIT, ASSIST, ONBOARDING, AUTOTUNE,
              PROFILES } s_active = HOME;

void settings_refresh() {
  settings_update(s_useF, s_muted, s_bright, s_tz, s_stainless);
}
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
  // LAZY CREATION: only the home screen is built at boot. Every other screen
  // is created on first show. Eagerly building all ~11 screens (~260 LVGL
  // objects) exhausted the basic board's 26 KB LVGL heap during init — and
  // LV_USE_ASSERT_MALLOC's default handler is while(1), i.e. a boot hang. The
  // simulator renders one screen per scene, so screenshots never exercised
  // the all-at-once footprint. Screens are still never destroyed; worst-case
  // steady state (user visits everything) is logged at boot + tracked in
  // HARDWARE_TEST.
  s_home = home_create();
  lv_scr_load(s_home);
  s_active = HOME;
}

void set_autotune_cb(AutotuneCb onAutotune) { s_autotuneCb = onAutotune; }
void show_autotune() {
  if (!s_autotune) s_autotune = autotune_create();
  lv_scr_load(s_autotune);
  s_active = AUTOTUNE;
}
void autotune_cmd(uint8_t cmd) {
  if (s_autotuneCb) s_autotuneCb(cmd);
  if (cmd == 2) show_settings();          // Cancel/Discard/Back -> Settings
}

void set_roi_cb(RoiCb onRoi) { s_roiCb = onRoi; }
void roi_lock(float px, float py) { if (s_roiCb) s_roiCb(px, py, true); }
void roi_clear() { if (s_roiCb) s_roiCb(0, 0, false); }

void set_profiles(const ProfileStore* store, ProfileCb onProfile) {
  s_profiles = store;
  s_profileCb = onProfile;
}
void show_profiles() {
  lv_obj_t* scr = profiles_create();   // idempotent; create once
  if (!scr) return;                    // heap-exhausted create: stay put
  if (s_profiles) profiles_update(*s_profiles);
  lv_scr_load(scr);
  s_active = PROFILES;
}
void profile_cmd(uint8_t cmd, int idx) {
  if (s_profileCb) s_profileCb(cmd, idx);
  if (s_profiles) profiles_update(*s_profiles);   // reflect the mutation
}

void set_onboarding_cb(OnboardingDoneCb onDone) { s_onboardCb = onDone; }
void show_onboarding() {
  if (!s_onboarding) s_onboarding = onboarding_create();
  onboarding_reset(s_useF);
  lv_scr_load(s_onboarding);
  s_active = ONBOARDING;
}
void onboarding_finish() { if (s_onboardCb) s_onboardCb(); show_home(); }

void set_preset_edit_cbs(PresetSaveCb onSave, PresetDeleteCb onDelete) {
  s_presetSaveCb = onSave;
  s_presetDelCb = onDelete;
}
void set_assist_cb(AssistCb onAssist) { s_assistCb = onAssist; }

void set_settings_cbs(MuteCb onMute, BrightnessCb onBrightness, TimezoneCb onTimezone,
                      StainlessCb onStainless) {
  s_muteCb = onMute;
  s_brightCb = onBrightness;
  s_tzCb = onTimezone;
  s_stainlessCb = onStainless;
}

void set_feedback_cb(FeedbackCb onFeedback) { s_feedbackCb = onFeedback; }
void set_food2_cb(FoodCb onFood2) { s_food2Cb = onFood2; }
void food_feedback(uint8_t verdict) { if (s_feedbackCb) s_feedbackCb(verdict); }

void show_home() { if (s_home) { lv_scr_load(s_home); s_active = HOME; } }
void show_thermal() { if (!s_thermal) s_thermal = thermal_create(); lv_scr_load(s_thermal); s_active = THERMAL; }

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

void show_learn() { if (!s_learn) s_learn = learn_create(); lv_scr_load(s_learn); s_active = LEARN; }
void show_lastcook() { if (!s_lastcook) s_lastcook = lastcook_create(); lv_scr_load(s_lastcook); s_active = LASTCOOK; }
void show_foods() { if (!s_foods) s_foods = foods_create(); s_foodZone = 0; lv_scr_load(s_foods); s_active = FOODS; }
void show_foods_zone2() { if (!s_foods) s_foods = foods_create(); s_foodZone = 1; lv_scr_load(s_foods); s_active = FOODS; }
void cook_a_food() { (s_presetZone == 1) ? show_foods_zone2() : show_foods(); }
void show_presets() { if (!s_presets) s_presets = presets_create(); s_presetZone = 0; presets_refresh(); lv_scr_load(s_presets); s_active = PRESETS; }
void show_presets_zone2() { if (!s_presets) s_presets = presets_create(); s_presetZone = 1; presets_refresh(); lv_scr_load(s_presets); s_active = PRESETS; }
void show_settings() {
  if (!s_settings) s_settings = settings_create();
  settings_refresh();
  lv_scr_load(s_settings);
  s_active = SETTINGS;
}

void show_preset_edit(int id) {
  if (!s_preset_edit) s_preset_edit = preset_edit_create();
  s_editId = id;
  s_editReturnZone = s_presetZone;   // remember which pan's picker we came from
  if (id >= 0 && presets_is_custom((uint8_t)id)) {
    const Preset& p = preset((uint8_t)id);
    preset_edit_load(p.name, p.loF, p.hiF, p.stainlessHints, /*canDelete=*/true);
  } else {
    s_editId = -1;
    preset_edit_load("", 300, 350, false, /*canDelete=*/false);
  }
  lv_scr_load(s_preset_edit);
  s_active = PRESET_EDIT;
}
namespace {
// Return to the picker the editor was opened from. Round-tripping through
// show_presets() used to reset the zone, so editing a preset mid-Pan-2 flow
// silently retargeted the next card tap at Pan 1.
void preset_edit_close() {
  (s_editReturnZone == 1) ? show_presets_zone2() : show_presets();
}
}  // namespace
void preset_edit_save(const char* name, int loF, int hiF, bool stainless) {
  if (s_presetSaveCb) s_presetSaveCb(s_editId, name, loF, hiF, stainless);
  preset_edit_close();
}
void preset_edit_delete() {
  if (s_editId >= 0 && s_presetDelCb) s_presetDelCb(s_editId);
  preset_edit_close();
}
void preset_edit_cancel() { preset_edit_close(); }

void show_assist_arm() {
  if (!s_assist) s_assist = assist_create();
  assist_load(s_actuatorName, s_assistAvail);
  lv_scr_load(s_assist);
  s_active = ASSIST;
}
void assist_arm() { if (s_assistCb) s_assistCb(0); show_home(); }
void assist_stop() { if (s_assistCb) s_assistCb(1); }

void toggle_unit() { s_useF = !s_useF; if (s_unitCb) s_unitCb(s_useF); }
void set_display_unit(bool useF) { s_useF = useF; if (s_unitCb) s_unitCb(s_useF); }

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
void settings_set_timezone(uint8_t idx) {
  const uint8_t nz = (uint8_t)tz_clamp(idx);
  if (s_tzCb) s_tzCb(nz);
  s_tz = nz;
  settings_refresh();
}
void settings_toggle_stainless() {
  const bool ns = !s_stainless;
  if (s_stainlessCb) s_stainlessCb(ns);
  s_stainless = ns;
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
void select_food(int id) {
  if (s_foodZone == 1 && s_food2Cb) s_food2Cb(id);
  else if (s_foodCb) s_foodCb(id);
  s_foodZone = 0;
  show_home();
}
void recipe_cmd(uint8_t cmd) { if (s_recipeCb) s_recipeCb(cmd); }
void start_recipe() { if (s_recipeCb) s_recipeCb(0); show_home(); }

void root_update(const ThermalFrame& f, const PanReading& r, const UiState& s) {
  s_muted = s.muted;                 // keep Settings' cache in sync with device truth
  s_bright = s.brightnessLevel;
  s_tz = s.tzIndex;
  s_stainless = s.stainlessPan;
  s_assistAvail = s.actuatorAvailable;
  s_actuatorName = s.actuatorName;
  if (s_active == HOME) home_update(s, s_useF);
  else if (s_active == THERMAL) thermal_update(f, r, s_useF);
  else if (s_active == LEARN) learn_update(s);
  else if (s_active == SETTINGS) settings_refresh();
  else if (s_active == AUTOTUNE) autotune_update(s);
  // presets / idle screens are static; nothing to update per tick
}

}  // namespace ui
