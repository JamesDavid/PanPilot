// app_state.h — the compact snapshot the UI renders (base spec §4 AppState,
// M2 subset). Hardware-free. Grows with guidance/ETA/preset at M3+.
#pragma once
#include <stdint.h>
#include "pan_types.h"
#include "core/thermal_model.h"
#include "core/guidance.h"
#include "core/presets.h"

enum class Mode : uint8_t { THERMOMETER, TARGET, PRESET };

struct UiState {
  Mode mode = Mode::TARGET;
  PanPresence presence = PanPresence::ABSENT;
  bool  modelValid = false;
  float displayTempC = 0;
  float rateCPerMin = 0;
  Trend trend = Trend::STABLE;
  uint8_t confidence = 0;
  bool  moved = false;      // CHECK_AIM (base spec §6.4 MOVED)
  bool  stainlessHint = false;
  bool  useF = true;        // display unit (persisted, base spec §4)
  bool  muted = false;

  // Target Assist (M3) + presets (M4)
  GuidanceState guidance = GuidanceState::IDLE;
  int   targetCenterF = 350;
  int   etaSeconds = -1;    // -1 = estimating/unknown
  float projectedPeakF = 0;
  uint8_t presetId = PRESET_GENERIC;
};
