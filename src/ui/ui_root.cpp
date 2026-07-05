// ui_root.cpp — see ui_root.h.
#include "ui_root.h"

#include <lvgl.h>

#include "ui/screen_home.h"
#include "ui/screen_thermal.h"
#include "ui/screen_presets.h"

namespace ui {
namespace {
lv_obj_t* s_home = nullptr;
lv_obj_t* s_thermal = nullptr;
lv_obj_t* s_presets = nullptr;
bool s_useF = true;
UnitChangeCb s_unitCb = nullptr;
TargetDeltaCb s_targetCb = nullptr;
PresetCb s_presetCb = nullptr;
enum Active { HOME, THERMAL, PRESETS } s_active = HOME;
}  // namespace

void root_init(bool useF, UnitChangeCb onUnit, TargetDeltaCb onTargetDelta,
               PresetCb onPreset) {
  s_useF = useF;
  s_unitCb = onUnit;
  s_targetCb = onTargetDelta;
  s_presetCb = onPreset;
  s_home = home_create();
  s_thermal = thermal_create();
  s_presets = presets_create();
  lv_scr_load(s_home);
  s_active = HOME;
}

void show_home() { if (s_home) { lv_scr_load(s_home); s_active = HOME; } }
void show_thermal() { if (s_thermal) { lv_scr_load(s_thermal); s_active = THERMAL; } }
void show_presets() { if (s_presets) { lv_scr_load(s_presets); s_active = PRESETS; } }

void toggle_unit() { s_useF = !s_useF; if (s_unitCb) s_unitCb(s_useF); }
bool unit_useF() { return s_useF; }
void target_adjust(int deltaF) { if (s_targetCb) s_targetCb(deltaF); }
void select_preset(uint8_t id) { if (s_presetCb) s_presetCb(id); show_home(); }

void root_update(const ThermalFrame& f, const PanReading& r, const UiState& s) {
  if (s_active == HOME) home_update(s, s_useF);
  else if (s_active == THERMAL) thermal_update(f, r, s_useF);
  // presets screen is static; nothing to update per tick
}

}  // namespace ui
