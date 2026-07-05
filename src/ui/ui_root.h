// ui_root.h — LVGL screen manager (base spec §3 ui/). Owns the home + thermal
// screens, the display-unit preference, and the target setpoint; routes the
// per-tick snapshot to the active screen. Board-agnostic so the simulator reuses it.
#pragma once
#include "pan_types.h"
#include "core/app_state.h"

namespace ui {

using UnitChangeCb = void (*)(bool useF);      // user toggled °F/°C
using TargetChangeCb = void (*)(int centerF);  // user changed the target

void root_init(bool useF, int targetCenterF, UnitChangeCb onUnit,
               TargetChangeCb onTarget);
void root_update(const ThermalFrame& f, const PanReading& r, const UiState& s);

void show_home();
void show_thermal();
void toggle_unit();
bool unit_useF();

void target_adjust(int deltaF);   // +/- step, clamped; fires TargetChangeCb
int  target_center();

}  // namespace ui
