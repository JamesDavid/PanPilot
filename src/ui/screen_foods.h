// screen_foods.h — food picker (roadmap §2.7). Scrollable list of the seed cook
// database; tap a food to arm its timer + target, tap its star to (un)favorite
// it (favorites sort first here and surface as cards on the preset picker).
// Portable LVGL; owns its screen.
#pragma once
#include <lvgl.h>

namespace ui {
lv_obj_t* foods_create();
void foods_refresh();   // rebuild rows (favorites first; star states current)
}
