// ui_root.h — LVGL screen manager (base spec §3 ui/). Owns the home / thermal /
// presets screens and the display-unit preference; routes the per-tick snapshot
// to the active screen. The target setpoint is owned by the logic layer — the UI
// only sends adjust/select commands and displays what comes back in UiState.
#pragma once
#include "pan_types.h"
#include "core/app_state.h"

class ProfileStore;

namespace ui {

using UnitChangeCb = void (*)(bool useF);      // user toggled °F/°C
using TargetDeltaCb = void (*)(int deltaF);    // user nudged the target +/-
using PresetCb = void (*)(uint8_t presetId);   // user picked a preset
using LearnCb = void (*)(uint8_t cmd);         // 0=start, 1=save, 2=cancel
using FoodCb = void (*)(int foodId);           // user picked a food (-1 = clear)
using RecipeCb = void (*)(uint8_t cmd);        // 0=start, 1=stop, 2=ack cue
using MuteCb = void (*)(bool muted);           // Settings: sound on/off
using BrightnessCb = void (*)(uint8_t level);  // Settings: backlight 0/1/2
using TimezoneCb = void (*)(uint8_t tzIndex);  // Settings: NTP timezone
using FeedbackCb = void (*)(uint8_t verdict);  // post-cook: 0=under,1=perfect,2=over
// Preset editor (Phase 2). editId < 0 => add a new custom preset.
using PresetSaveCb = void (*)(int editId, const char* name, int loF, int hiF, bool stainless);
using PresetDeleteCb = void (*)(int id);
using AssistCb = void (*)(uint8_t cmd);   // ASSIST: 0=arm, 1=stop/disarm
using OnboardingDoneCb = void (*)();      // first-boot wizard finished
using AutotuneCb = void (*)(uint8_t cmd); // PID autotune: 0=start, 1=save, 2=cancel
using RoiCb = void (*)(float px, float py, bool lock);  // thermal tap-to-lock ROI
using ProfileCb = void (*)(uint8_t cmd, int idx);       // pans: 0=activate, 1=delete

void root_init(bool useF, UnitChangeCb onUnit, TargetDeltaCb onTargetDelta,
               PresetCb onPreset, LearnCb onLearn, FoodCb onFood, PresetCb onPreset2,
               RecipeCb onRecipe);
void root_update(const ThermalFrame& f, const PanReading& r, const UiState& s);

void show_home();
void show_thermal();
void show_presets();
void show_idle();     // dim "monitoring" screen (base spec §8)
void show_learn();    // Learn Pan Mode wizard (base spec §7 Phase 1.5)
void show_lastcook(); // Last Cook summary (roadmap §2.3)
void show_foods();    // food picker for the primary pan (roadmap §2.7)
void show_foods_zone2();    // food picker for the second pan (Phase 3)
void cook_a_food();   // food picker for whichever pan's preset picker is open
void show_presets_zone2();  // preset picker that sets zone-2's target (M12)
void show_settings(); // device Settings (Phase 2)
void set_settings_cbs(MuteCb onMute, BrightnessCb onBrightness, TimezoneCb onTimezone);
void set_food2_cb(FoodCb onFood2);   // second-pan food selection (Phase 3)
void set_feedback_cb(FeedbackCb onFeedback);
void food_feedback(uint8_t verdict);   // fires FeedbackCb (0=under,1=perfect,2=over)
void set_preset_edit_cbs(PresetSaveCb onSave, PresetDeleteCb onDelete);
void show_preset_edit(int id);         // id < 0 => new custom preset
void preset_edit_save(const char* name, int loF, int hiF, bool stainless);
void preset_edit_delete();
void preset_edit_cancel();             // back to the picker it was opened from
void set_assist_cb(AssistCb onAssist);
void show_assist_arm();                 // open the ASSIST arming ceremony
void assist_arm();                      // fires AssistCb(0), returns home
void assist_stop();                     // fires AssistCb(1) — the STOP bar
void set_autotune_cb(AutotuneCb onAutotune);
void show_autotune();                   // PID autotune wizard
void autotune_cmd(uint8_t cmd);         // fires AutotuneCb (0=start,1=save,2=cancel)
void set_roi_cb(RoiCb onRoi);
void roi_lock(float px, float py);      // thermal tap: pin the ROI to a pixel
void roi_clear();                       // thermal "Auto": back to auto-follow
void set_profiles(const ProfileStore* store, ProfileCb onProfile);
void show_profiles();                   // saved pan-profile picker
void profile_cmd(uint8_t cmd, int idx); // fires ProfileCb (0=activate,1=delete)
void settings_toggle_unit();       // fires UnitChangeCb + refreshes Settings
void settings_toggle_mute();       // fires MuteCb + refreshes Settings
void settings_cycle_brightness();  // fires BrightnessCb + refreshes Settings
void settings_cycle_timezone();    // fires TimezoneCb + refreshes Settings
void toggle_unit();
void set_display_unit(bool useF);   // set unit directly (onboarding); fires UnitChangeCb
bool unit_useF();
void set_onboarding_cb(OnboardingDoneCb onDone);
void show_onboarding();             // first-boot wizard
void onboarding_finish();           // fires OnboardingDoneCb, returns home
void target_adjust(int deltaF);     // fires TargetDeltaCb
void select_preset(uint8_t id);     // fires PresetCb, returns home
void learn_cmd(uint8_t cmd);        // fires LearnCb
void select_food(int id);           // fires FoodCb, returns home
void recipe_cmd(uint8_t cmd);       // fires RecipeCb (0=start,1=stop,2=ack)
void start_recipe();                // start the built-in recipe, return home

}  // namespace ui
