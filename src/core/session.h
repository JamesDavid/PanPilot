// session.h — accumulate a cook session summary (base spec §8, §10). Hardware-
// free, unit-tested. One summary per cook is stored to an NVS ring (last 10).
#pragma once
#include <stdint.h>
#include "pan_types.h"
#include "core/guidance.h"

struct SessionSummary {
  uint16_t version;         // struct version tag (base spec §10)
  uint8_t  presetId;
  uint8_t  minConfidence;
  float    maxTempC;
  uint32_t startMs;
  uint32_t endMs;
  uint32_t timeInRangeSec;
  uint32_t overheatSec;
  int32_t  timeToTargetSec;  // -1 if target never reached
  uint16_t foodAddedCount;   // M6
};

class SessionAccumulator {
 public:
  void begin(uint8_t presetId, uint32_t now_ms);
  // Advance with the latest reading + guidance state. dt is derived from t_ms.
  void update(const PanReading& r, GuidanceState g, uint32_t now_ms);
  void foodAdded() { if (active_) ++s_.foodAddedCount; }
  SessionSummary finish(uint32_t now_ms);
  bool active() const { return active_; }

 private:
  bool active_ = false;
  uint32_t last_ms_ = 0;
  SessionSummary s_{};
  bool reached_range_ = false;
};
