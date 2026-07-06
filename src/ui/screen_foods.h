// screen_foods.h — food picker (roadmap §2.7). Scrollable list of the seed cook
// database; tap a food to arm its timer + target. Portable LVGL; owns its screen.
#pragma once
#include <lvgl.h>

namespace ui {
lv_obj_t* foods_create();
}
