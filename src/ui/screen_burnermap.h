// screen_burnermap.h — "Map Burner" wizard (per-pan knob calibration). Walks
// the user through 5 knob settings + one burner-off cooling window (~5 min);
// the solved map is stored on the ACTIVE pan profile and replaces the generic
// knob table for that pan's down-cue hints. Portable LVGL; owns its screen.
#pragma once
#include <lvgl.h>
#include "core/app_state.h"

namespace ui {
lv_obj_t* burnermap_create();
void burnermap_update(const UiState& s, bool useF);
}
