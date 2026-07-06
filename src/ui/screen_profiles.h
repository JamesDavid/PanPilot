// screen_profiles.h — saved pan-profile picker (Phase 3, "8 profiles not 1").
// Lists the Learn-Pan profiles with the active one marked; tap to activate,
// or delete. Portable LVGL; owns its screen. Rebuilt from a ProfileStore.
#pragma once
#include <lvgl.h>

class ProfileStore;

namespace ui {
lv_obj_t* profiles_create();
void profiles_update(const ProfileStore& ps);
}
