// recovery.h — FOOD ADDED detection + Recovery Monitor (base spec §6.4, §7.4).
// Hardware-free, unit-tested. Detects the temperature drop when food hits the
// pan, then watches it climb back into the target band and cues the next batch.
#pragma once
#include <stdint.h>
#include "app_config.h"

enum class RecoveryEvent : uint8_t { NONE, FOOD_ADDED, RECOVERED };
enum class RecoveryHint  : uint8_t { NONE, SLOW, FAST };  // raise/watch heat

struct RecoveryOut {
  bool recovering = false;
  RecoveryEvent event = RecoveryEvent::NONE;
  RecoveryHint hint = RecoveryHint::NONE;
};

class RecoveryMonitor {
 public:
  // enabled = the active preset has recovery monitoring on (§7.4).
  // targetLoF = bottom of the ready band to recover to.
  RecoveryOut update(float tempF, float rateFPerMin, bool present,
                     float targetLoF, bool enabled, uint32_t now_ms);
  void reset();
  bool recovering() const { return recovering_; }

 private:
  // short history for the drop test (~3 s)
  static constexpr int H = 16;
  float ht_[H]; uint32_t hms_[H];
  int hn_ = 0, hhead_ = 0;
  bool recovering_ = false;
  float preDropF_ = 0;
  uint32_t enteredMs_ = 0;
  void pushHist(float t, uint32_t ms);
  float tempAgo(uint32_t now, uint32_t ago_ms) const;
};
