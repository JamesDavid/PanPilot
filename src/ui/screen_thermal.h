// screen_thermal.h — live thermal view: aiming + ROI screen (base spec §9.2).
// Portable LVGL; owns its own screen object (ui_root switches between screens).
#pragma once
#include <lvgl.h>
#include "pan_types.h"

namespace ui {

lv_obj_t* thermal_create();     // build + return the thermal screen object
void thermal_update(const ThermalFrame& f, const PanReading& r, bool useF);

}  // namespace ui
