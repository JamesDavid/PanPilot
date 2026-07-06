// screen_onboarding.h — first-boot setup wizard (base spec §4 first-run). A few
// swipe-free steps: welcome, temperature unit, mount/aim guidance, and a ready
// screen. Shown once (a persisted flag gates it); finishing hands control back
// to the home screen. Portable LVGL; owns its screen.
#pragma once
#include <lvgl.h>

namespace ui {
lv_obj_t* onboarding_create();
void onboarding_reset(bool useF);   // start at step 0 with the current unit
}
