// ui_root.cpp — see ui_root.h.
#include "ui_root.h"

#include <lvgl.h>
#include <algorithm>

#include "ui/screen_home.h"
#include "ui/screen_thermal.h"
#include "app_config.h"

namespace ui {
namespace {
lv_obj_t* s_home = nullptr;
lv_obj_t* s_thermal = nullptr;
bool s_useF = true;
int  s_targetCenter = TARGET_DEFAULT_CENTER_F;
UnitChangeCb s_unitCb = nullptr;
TargetChangeCb s_targetCb = nullptr;
enum Active { HOME, THERMAL } s_active = HOME;
}  // namespace

void root_init(bool useF, int targetCenterF, UnitChangeCb onUnit,
               TargetChangeCb onTarget) {
  s_useF = useF;
  s_targetCenter = targetCenterF;
  s_unitCb = onUnit;
  s_targetCb = onTarget;
  s_home = home_create();
  s_thermal = thermal_create();
  lv_scr_load(s_home);
  s_active = HOME;
}

void show_home() { if (s_home) { lv_scr_load(s_home); s_active = HOME; } }
void show_thermal() { if (s_thermal) { lv_scr_load(s_thermal); s_active = THERMAL; } }

void toggle_unit() {
  s_useF = !s_useF;
  if (s_unitCb) s_unitCb(s_useF);
}
bool unit_useF() { return s_useF; }

void target_adjust(int deltaF) {
  s_targetCenter = std::max(100, std::min(600, s_targetCenter + deltaF));
  if (s_targetCb) s_targetCb(s_targetCenter);
}
int target_center() { return s_targetCenter; }

void root_update(const ThermalFrame& f, const PanReading& r, const UiState& s) {
  if (s_active == HOME) home_update(s, s_useF);
  else thermal_update(f, r, s_useF);
}

}  // namespace ui
