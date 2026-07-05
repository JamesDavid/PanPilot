// screen_home.h — home / cooking screen (base spec §9.1). M2 Thermometer Mode:
// big smoothed temperature, rate + trend arrow, presence/aim state, status bar
// with confidence + unit toggle. Portable LVGL; owns its screen object.
#pragma once
#include <lvgl.h>
#include "core/app_state.h"

namespace ui {

lv_obj_t* home_create();
void home_update(const UiState& s, bool useF);

}  // namespace ui
