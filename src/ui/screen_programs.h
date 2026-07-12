// screen_programs.h — recipe-program picker: the built-in program plus every
// program saved from the web Recipe Creator (/programs on LittleFS). Tap to
// run through the sequencer. The list content comes from the loop core via
// ui::programs_show (the UI layer never touches the filesystem). Portable
// LVGL; owns its screen.
#pragma once
#include <lvgl.h>

namespace ui {
lv_obj_t* programs_create();
// Rebuild rows: built-in first, then `n` saved names (each < 24 chars).
void programs_set_list(const char (*names)[24], int n);
const char* programs_name(int i);   // saved name by index ("" out of range)
}
