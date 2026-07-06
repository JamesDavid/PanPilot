// screen_autotune.h — PID autotune wizard (roadmap §3.2). Runs the relay
// autotuner (which drives the armed actuator) and shows progress, then the
// derived gains to accept or discard. Available only while ASSIST is armed.
// Portable LVGL; owns its screen. State comes from UiState.autotune*.
#pragma once
#include <lvgl.h>
#include "core/app_state.h"

namespace ui {
lv_obj_t* autotune_create();
void autotune_update(const UiState& s);
}
