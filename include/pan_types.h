// pan_types.h — core data structs shared across sensor/core/ui (base spec §4).
// Hardware-free: plain values only, no Arduino/LVGL. All temperatures °C
// (conversion to °F happens at the display layer).
#pragma once
#include <stdint.h>

constexpr int THERM_ROWS = 24;
constexpr int THERM_COLS = 32;
constexpr int THERM_PIXELS = THERM_ROWS * THERM_COLS;  // 768

// Raw-ish sensor output (one assembled full frame).
struct ThermalFrame {
  float px[THERM_ROWS][THERM_COLS];  // °C per pixel
  float ambientC;                    // sensor die / ambient
  uint32_t t_ms;
  bool valid;
};

enum class PanPresence : uint8_t { ABSENT, PRESENT, OBSTRUCTED, UNCERTAIN };

// Output of frame_analysis — what guidance + UI consume (base spec §6).
struct PanReading {
  float panTempC;       // robust ROI statistic (§6.2: 75th pct of interior)
  float roiMinC;
  float roiMaxC;
  float backgroundC;    // scene median excluding ROI
  uint16_t roiPixelCount;
  float roiCx;          // blob centroid, pixel coords (col)
  float roiCy;          // blob centroid, pixel coords (row)
  uint8_t confidence;   // 0..100 (§6.3)
  PanPresence presence;
  bool stainlessHint;   // reflective-stainless signature (§6.3 / §7.5)
  bool moved;           // centroid jumped > threshold -> CHECK AIM (§6.4)
  uint32_t t_ms;
};
