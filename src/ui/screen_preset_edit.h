// screen_preset_edit.h — create/edit a custom preset (Phase 2). Name (on-screen
// keyboard), low/high °F steppers, and a stainless toggle. Portable LVGL; owns
// its screen. Commits through ui_root (preset_edit_save / preset_edit_delete).
#pragma once
#include <lvgl.h>

namespace ui {
lv_obj_t* preset_edit_create();
// Prefill the form. canDelete shows the Delete button (edit vs. new).
void preset_edit_load(const char* name, int loF, int hiF, bool stainless,
                      bool canDelete);
}
