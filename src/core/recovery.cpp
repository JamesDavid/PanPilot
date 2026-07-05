// recovery.cpp — see recovery.h.
#include "recovery.h"

namespace {
constexpr float kDropF = FOOD_DROP_C * 9.0f / 5.0f;  // ~27 °F
}

void RecoveryMonitor::reset() {
  hn_ = hhead_ = 0; recovering_ = false; preDropF_ = 0; enteredMs_ = 0;
}

void RecoveryMonitor::pushHist(float t, uint32_t ms) {
  const int i = (hhead_ + hn_) % H;
  if (hn_ < H) { ht_[i] = t; hms_[i] = ms; ++hn_; }
  else { ht_[hhead_] = t; hms_[hhead_] = ms; hhead_ = (hhead_ + 1) % H; }
}

// Temperature closest to (now - ago_ms). Falls back to the oldest sample.
float RecoveryMonitor::tempAgo(uint32_t now, uint32_t ago_ms) const {
  const uint32_t want = now - ago_ms;
  float best = ht_[hhead_]; uint32_t bestd = 0xFFFFFFFF; bool found = false;
  for (int k = 0; k < hn_; ++k) {
    const int idx = (hhead_ + k) % H;
    const uint32_t d = hms_[idx] > want ? hms_[idx] - want : want - hms_[idx];
    if (d < bestd) { bestd = d; best = ht_[idx]; found = true; }
  }
  return found ? best : ht_[(hhead_ + hn_ - 1) % H];
}

RecoveryOut RecoveryMonitor::update(float tempF, float rateFPerMin, bool present,
                                    float targetLoF, bool enabled,
                                    uint32_t now_ms) {
  RecoveryOut out;
  if (!enabled) { reset(); return out; }
  if (present) pushHist(tempF, now_ms);

  if (!recovering_) {
    if (present && hn_ >= 2) {
      const float past = tempAgo(now_ms, FOOD_ADDED_WINDOW_MS);
      if (past - tempF >= kDropF) {       // sudden drop => food added
        recovering_ = true; preDropF_ = past; enteredMs_ = now_ms;
        out.event = RecoveryEvent::FOOD_ADDED;
      }
    }
  }

  if (recovering_) {
    out.recovering = true;
    if (tempF >= targetLoF) {             // climbed back into the band
      recovering_ = false;
      out.event = RecoveryEvent::RECOVERED;
    } else if (rateFPerMin > RECOVERY_FAST_FMIN) {
      out.hint = RecoveryHint::FAST;      // watch the heat
    } else if (rateFPerMin > 1.0f) {
      const float sec = (targetLoF - tempF) / rateFPerMin * 60.0f;
      if (sec > RECOVERY_SLOW_SEC) out.hint = RecoveryHint::SLOW;  // raise heat
    } else {
      out.hint = RecoveryHint::SLOW;      // barely climbing
    }
  }
  return out;
}
