// power.h — battery/power HAL (roadmap spec §2.1). Device-only. Reads the
// MAX17048 fuel gauge over the shared I2C bus and reports USB-present. Degrades
// gracefully to "USB, unknown SoC" when no gauge is fitted (BatteryState.valid
// stays false), so the same binary runs on USB-only builds.
#pragma once
#if !defined(PANPILOT_SIM)
#include "core/battery.h"

namespace hal {
void power_begin();
BatteryState power_read();   // poll fuel gauge + USB detect
}  // namespace hal
#endif
