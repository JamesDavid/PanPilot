// battery.cpp — see battery.h.
#include "battery.h"

BattEvent BatteryMonitor::update(const BatteryState& s) {
  if (!s.valid) return BattEvent::NONE;
  if (s.usbPresent) {                 // on USB: no low alerts; re-arm for later
    low_fired_ = crit_fired_ = false;
    return BattEvent::NONE;
  }
  // Re-arm when SoC recovers above the band (hysteresis).
  if (s.soc > BATTERY_LOW_PCT + BATTERY_REARM_PCT) low_fired_ = false;
  if (s.soc > BATTERY_CRITICAL_PCT + BATTERY_REARM_PCT) crit_fired_ = false;

  if (s.soc <= BATTERY_CRITICAL_PCT && !crit_fired_) {
    crit_fired_ = true;
    return BattEvent::CRITICAL;
  }
  if (s.soc <= BATTERY_LOW_PCT && !low_fired_) {
    low_fired_ = true;
    return BattEvent::LOW_BATT;
  }
  return BattEvent::NONE;
}
