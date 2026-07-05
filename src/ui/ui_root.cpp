// ui_root.cpp — see ui_root.h. Thin screen manager over screen_home /
// screen_thermal (which own their LVGL screen objects).
#include "ui_root.h"

#include <lvgl.h>

#include "ui/screen_home.h"
#include "ui/screen_thermal.h"

namespace ui {
namespace {
lv_obj_t* s_home = nullptr;
lv_obj_t* s_thermal = nullptr;
bool s_useF = true;
UnitChangeCb s_cb = nullptr;
enum Active { HOME, THERMAL } s_active = HOME;
}  // namespace

void root_init(bool useF, UnitChangeCb cb) {
  s_useF = useF;
  s_cb = cb;
  s_home = home_create();
  s_thermal = thermal_create();
  lv_scr_load(s_home);
  s_active = HOME;
}

void show_home() {
  if (s_home) { lv_scr_load(s_home); s_active = HOME; }
}
void show_thermal() {
  if (s_thermal) { lv_scr_load(s_thermal); s_active = THERMAL; }
}
void toggle_unit() {
  s_useF = !s_useF;
  if (s_cb) s_cb(s_useF);
}
bool unit_useF() { return s_useF; }

void root_update(const ThermalFrame& f, const PanReading& r, const UiState& s) {
  if (s_active == HOME) home_update(s, s_useF);
  else thermal_update(f, r, s_useF);
}

}  // namespace ui
