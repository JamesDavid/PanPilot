// session.cpp — see session.h.
#include "session.h"
#include <algorithm>

constexpr uint16_t SESSION_STRUCT_VERSION = 1;

void SessionAccumulator::begin(uint8_t presetId, uint32_t now_ms) {
  s_ = SessionSummary{};
  s_.version = SESSION_STRUCT_VERSION;
  s_.presetId = presetId;
  s_.minConfidence = 100;
  s_.maxTempC = -1000.0f;
  s_.startMs = now_ms;
  s_.timeToTargetSec = -1;
  last_ms_ = now_ms;
  reached_range_ = false;
  active_ = true;
}

void SessionAccumulator::update(const PanReading& r, GuidanceState g,
                                uint32_t now_ms) {
  if (!active_) return;
  const float dt = (now_ms - last_ms_) / 1000.0f;
  last_ms_ = now_ms;
  if (dt <= 0) return;

  if (r.presence == PanPresence::PRESENT) {
    s_.maxTempC = std::max(s_.maxTempC, r.panTempC);
    s_.minConfidence = std::min<uint8_t>(s_.minConfidence, r.confidence);
  }
  if (g == GuidanceState::READY) {
    s_.timeInRangeSec += (uint32_t)(dt + 0.5f);
    if (!reached_range_) {
      reached_range_ = true;
      s_.timeToTargetSec = (int32_t)((now_ms - s_.startMs) / 1000);
    }
  } else if (g == GuidanceState::TOO_HOT) {
    s_.overheatSec += (uint32_t)(dt + 0.5f);
  }
}

SessionSummary SessionAccumulator::finish(uint32_t now_ms) {
  s_.endMs = now_ms;
  active_ = false;
  return s_;
}
