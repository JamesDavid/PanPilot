// display.h — device display + touch HAL. Owns LovyanGFX and wires it into LVGL
// (draw buffers, flush callback, GT911 touch input device). Board-specific glue;
// the ui/ layer draws on top via standard LVGL calls (base spec §3, §9).
#pragma once

namespace hal {

// Init panel, backlight, LVGL display + touch input. Call once from setup(),
// after Serial. Safe no-op flush target until LVGL is running.
void display_begin();

// Set backlight 0..255 (base spec §8 ACTIVE/IDLE brightness).
void display_set_brightness(uint8_t level);

}  // namespace hal
