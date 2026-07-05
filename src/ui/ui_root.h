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

void root_init(bool useF, UnitChangeCb onUnit, TargetDeltaCb onTargetDelta,
               PresetCb onPreset);
void root_update(const ThermalFrame& f, const PanReading& r, const UiState& s);

void show_home();
void show_thermal();
void show_presets();
void show_idle();     // dim "monitoring" screen (base spec §8)
void toggle_unit();
bool unit_useF();
void target_adjust(int deltaF);     // fires TargetDeltaCb
void select_preset(uint8_t id);     // fires PresetCb, returns home

}  // namespace ui
