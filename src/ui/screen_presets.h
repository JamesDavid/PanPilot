// screen_presets.h — preset picker: grid of cards (base spec §9.3). Portable
// LVGL; owns its screen. Tapping a card selects that preset and returns home.
#pragma once
#include <lvgl.h>

namespace ui {
lv_obj_t* presets_create();
void presets_refresh();   // rebuild the card grid (custom presets changed)
}
