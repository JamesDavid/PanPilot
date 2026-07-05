// screen_learn.h — Learn Pan Mode wizard (base spec §7 Phase 1.5, §9.3).
// Portable LVGL; owns its screen. Records a pan's heating behavior to learn its
// thermal lag for better overshoot prediction.
#pragma once
#include <lvgl.h>
#include "core/app_state.h"

namespace ui {
lv_obj_t* learn_create();
void learn_update(const UiState& s);
}
