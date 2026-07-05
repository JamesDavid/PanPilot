// ui_root.h — LVGL screen manager (base spec §3 ui/). Owns the home + thermal
// screens and the display-unit preference; routes the per-tick snapshot to the
// active screen. Board-agnostic (LVGL only) so the simulator reuses it.
#pragma once
#include "pan_types.h"
#include "core/app_state.h"

namespace ui {

using UnitChangeCb = void (*)(bool useF);  // fired when the user toggles °F/°C

void root_init(bool useF, UnitChangeCb onUnitChange);
void root_update(const ThermalFrame& f, const PanReading& r, const UiState& s);

void show_home();
void show_thermal();
void toggle_unit();
bool unit_useF();

}  // namespace ui
