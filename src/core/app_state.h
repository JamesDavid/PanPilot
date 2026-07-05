// app_state.h — the compact snapshot the UI renders (base spec §4 AppState,
// M2 subset). Hardware-free. Grows with guidance/ETA/preset at M3+.
#pragma once
#include <stdint.h>
#include "pan_types.h"
#include "core/thermal_model.h"

enum class Mode : uint8_t { THERMOMETER, TARGET, PRESET };  // TARGET/PRESET: M3/M4

struct UiState {
  Mode mode = Mode::THERMOMETER;
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
};
