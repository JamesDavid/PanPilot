// screen_thermal.h — live thermal view: the aiming + ROI screen that replaces
// the laser (base spec §9.2). Portable LVGL (no Arduino), so the simulator
// renders the identical view from a synthetic frame.
#pragma once
#include "pan_types.h"

namespace ui {

// Build the thermal screen objects on the active screen (call once).
void thermal_create();

// Repaint the false-color image + overlays (crosshair, ROI, panTemp, aim hint)
// from a frame + its analysis. `useF` selects °F vs °C for the label.
void thermal_update(const ThermalFrame& f, const PanReading& r, bool useF);

}  // namespace ui
