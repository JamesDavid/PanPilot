// foodtimer.h — temperature-compensated cook timer (roadmap §2.7). The
// countdown is a *doneness accumulator*, not wall-clock: progress accrues as
// Δt × k(panTemp), so a cold pan stretches the time and a hot pan shortens it.
// Auto-starts on FOOD ADDED, cues FLIP/REMOVE, handles per-side + rest. The
// safety note for entries with safeInternalF is surfaced by the UI, never here.
// Hardware-free + unit-tested.
#pragma once
#include <stdint.h>
#include "core/foodlib.h"

struct FoodTimerOut {
  enum Phase : uint8_t { IDLE, COOKING, RESTING, DONE } phase = IDLE;
  enum Event : uint8_t { NONE, FLIP, REMOVE, REST_DONE } event = NONE;
  uint8_t side = 1;         // 1-based current side
  int remainingSec = 0;     // temp-compensated estimate for the current side
  float k = 1.0f;           // current compensation factor (for the banner)
};

class FoodTimer {
 public:
  void start(const FoodEntry* e, uint32_t now);   // e.g. on FOOD ADDED
  void stop();
  FoodTimerOut update(float panTempF, uint32_t now);
  bool active() const { return e_ != nullptr && !done_; }
  const FoodEntry* entry() const { return e_; }
  uint8_t side() const { return side_ + 1; }

  // k(panTemp): 1 at refTempF, scaled by tempCompPctPer25F, clamped 0.7..1.3.
  static float compFactor(const FoodEntry* e, float panTempF);

 private:
  const FoodEntry* e_ = nullptr;
  uint32_t last_ = 0;
  float progress_ = 0;      // doneness-seconds accrued for the current side
  uint8_t side_ = 0;        // 0-based
  bool resting_ = false;
  float restProg_ = 0;
  bool done_ = false;
};
