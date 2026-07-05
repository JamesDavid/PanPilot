// ui_root.h — LVGL screen manager entry point (base spec §3 ui/).
// Board-agnostic: no Arduino/LovyanGFX includes, only LVGL + hal interfaces, so
// the same UI compiles for device and the SDL simulator (kickoff §Screenshots).
#pragma once

namespace ui {

// Build the initial screen tree. Call after LVGL + display are initialized.
void root_init();

}  // namespace ui
