// battery.h — battery state + low/critical event logic (roadmap spec §2.1).
// Hardware-free + unit-tested; the hal/power layer feeds it raw SoC/charging
// from the fuel gauge (MAX17048). Events are edge-triggered with hysteresis so
// a SoC hovering at a threshold doesn't spam alerts.
#pragma once
#include <stdint.h>
#include "app_config.h"

enum class BattEvent : uint8_t { NONE, LOW_BATT, CRITICAL };

struct BatteryState {
  bool usbPresent = true;
  bool charging = false;
  uint8_t soc = 100;      // 0..100 %
  bool valid = false;     // false until a fuel-gauge reading arrives
};

class BatteryMonitor {
 public:
  // Returns an event on the falling edge into a threshold band (only on
  // battery — USB present suppresses low/critical).
  BattEvent update(const BatteryState& s);
  void reset() { low_fired_ = crit_fired_ = false; }

 private:
  bool low_fired_ = false;
  bool crit_fired_ = false;
};
