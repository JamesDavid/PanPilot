// screen_settings.h — device Settings (Phase 2). Aggregates the preferences that
// aren't part of a cook: temperature unit, sound, and backlight brightness, plus
// an About line. Portable LVGL; owns its screen. Rows tap to cycle/toggle and
// fire commands back through ui_root; settings_update() refreshes the values.
#pragma once
#include <lvgl.h>
#include <stdint.h>

namespace ui {
lv_obj_t* settings_create();
void settings_update(bool useF, bool muted, uint8_t brightnessLevel,
                     uint8_t tzIndex);
}
