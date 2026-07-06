// screen_assist.h — ASSIST arming ceremony (roadmap §3.3). Handing burner power
// to PanPilot is deliberate friction: the user must acknowledge they'll stay
// nearby before Autopilot can be armed. Until armed, PanPilot is pure ADVISORY
// and nothing here can energize an actuator. Portable LVGL; owns its screen.
#pragma once
#include <lvgl.h>

namespace ui {
lv_obj_t* assist_create();
void assist_load(const char* actuatorName, bool actuatorReady);
}
