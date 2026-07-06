// settings.h — user-adjustable device settings that don't belong to the cook
// itself: display unit, sound, and backlight brightness. Hardware-free so the
// mappings can be unit-tested; the UI reads/writes these and the HAL applies
// the PWM value. Persisted to NVS by hal/storage (device only).
#pragma once
#include <stdint.h>

namespace panpilot {

// Backlight brightness presets (0..2). Level maps to an 8-bit LEDC duty; the
// active-state backlight uses this instead of a single fixed constant so the
// user can dim a too-bright panel without going all the way to IDLE.
// NOTE: values are DIM/MID/FULL, not LOW/HIGH — Arduino defines LOW/HIGH as
// GPIO macros, and using them as enumerators fails to compile on-device.
enum class Brightness : uint8_t { DIM = 0, MID = 1, FULL = 2 };

inline uint8_t brightness_pwm(uint8_t level) {
  switch (level) {
    case 0:  return 64;    // ~25% — comfortable in a dark kitchen
    case 1:  return 150;   // ~59%
    default: return 255;   // full — bright range hood / daylight
  }
}

inline const char* brightness_name(uint8_t level) {
  switch (level) {
    case 0:  return "Low";
    case 1:  return "Medium";
    default: return "High";
  }
}

// Tap-to-cycle order Low -> Medium -> High -> Low.
inline uint8_t brightness_cycle(uint8_t level) {
  return (uint8_t)((level + 1) % 3);
}

inline uint8_t brightness_clamp(int level) {
  return (uint8_t)(level < 0 ? 0 : (level > 2 ? 2 : level));
}

}  // namespace panpilot
