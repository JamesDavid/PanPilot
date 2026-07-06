// compliance.h — knob-turn compliance verification (roadmap spec §3.5, Stovetop
// Advisor). After a "turn down"/"turn up" cue, watch whether the pan's rate of
// change actually responds within a window; if not, the caller escalates.
// Hardware-free + unit-tested.
#pragma once
#include <stdint.h>
#include "app_config.h"

enum class Compliance : uint8_t { IDLE, PENDING, COMPLIED, FAILED };

class ComplianceChecker {
 public:
  // Begin watching. expectFall=true after a "turn down" cue (rate should drop
  // below +COMPLY_RATE_FMIN); false after "turn up" (rate should rise above it).
  void start(bool expectFall, uint32_t now);
  Compliance update(float rateFPerMin, uint32_t now);
  Compliance state() const { return state_; }
  void reset() { state_ = Compliance::IDLE; }

 private:
  Compliance state_ = Compliance::IDLE;
  bool expectFall_ = true;
  uint32_t start_ms_ = 0;
};
