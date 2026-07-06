// ui_root.h — LVGL screen manager (base spec §3 ui/). Owns the home / thermal /
// presets screens and the display-unit preference; routes the per-tick snapshot
// to the active screen. The target setpoint is owned by the logic layer — the UI
// only sends adjust/select commands and displays what comes back in UiState.
#pragma once
#include "pan_types.h"
#include "core/app_state.h"

namespace ui {

using UnitChangeCb = void (*)(bool useF);      // user toggled °F/°C
using TargetDeltaCb = void (*)(int deltaF);    // user nudged the target +/-
using PresetCb = void (*)(uint8_t presetId);   // user picked a preset
using LearnCb = void (*)(uint8_t cmd);         // 0=start, 1=save, 2=cancel
using FoodCb = void (*)(int foodId);           // user picked a food (-1 = clear)
using RecipeCb = void (*)(uint8_t cmd);        // 0=start, 1=stop, 2=ack cue
using MuteCb = void (*)(bool muted);           // Settings: sound on/off
using BrightnessCb = void (*)(uint8_t level);  // Settings: backlight 0/1/2
using FeedbackCb = void (*)(uint8_t verdict);  // post-cook: 0=under,1=perfect,2=over

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
void show_foods();    // food picker (roadmap §2.7)
void show_presets_zone2();  // preset picker that sets zone-2's target (M12)
void show_settings(); // device Settings (Phase 2)
void set_settings_cbs(MuteCb onMute, BrightnessCb onBrightness);
void set_feedback_cb(FeedbackCb onFeedback);
void food_feedback(uint8_t verdict);   // fires FeedbackCb (0=under,1=perfect,2=over)
void settings_toggle_unit();       // fires UnitChangeCb + refreshes Settings
void settings_toggle_mute();       // fires MuteCb + refreshes Settings
void settings_cycle_brightness();  // fires BrightnessCb + refreshes Settings
void toggle_unit();
bool unit_useF();
void target_adjust(int deltaF);     // fires TargetDeltaCb
void select_preset(uint8_t id);     // fires PresetCb, returns home
void learn_cmd(uint8_t cmd);        // fires LearnCb
void select_food(int id);           // fires FoodCb, returns home
void recipe_cmd(uint8_t cmd);       // fires RecipeCb (0=start,1=stop,2=ack)
void start_recipe();                // start the built-in recipe, return home

}  // namespace ui
