// thermal_model.h — smoothing, rate, and trend from the PanReading stream
// (base spec §7.1). Hardware-free, unit-tested. ETA + overshoot prediction are
// added at M3/M4; this is the M2 subset (display temp, rate, trend class).
#pragma once
#include <stdint.h>
#include "pan_types.h"
#include "app_config.h"

enum class Trend : uint8_t { STABLE, RISING_SLOW, RISING_FAST, COOLING };

class ThermalModel {
 public:
  // Feed one reading (only PRESENT/UNCERTAIN readings advance the model; absent
  // readings are ignored so a removed pan doesn't corrupt the trace).
  void update(const PanReading& r);
  void reset();

  bool  valid() const { return have_disp_; }
  float displayTempC() const { return disp_c_; }   // exp-smoothed (τ≈2 s)
  float rateCPerMin() const { return rate_c_min_; } // least-squares over window
  Trend trend() const;

  // °F helpers for the display layer.
  static float cToF(float c) { return c * 9.0f / 5.0f + 32.0f; }
  float displayTempF() const { return cToF(disp_c_); }
  float rateFPerMin() const { return rate_c_min_ * 9.0f / 5.0f; }

 private:
  static constexpr int CAP = 256;   // ~60 s at 4 Hz (base spec §7.1)
  float t_c_[CAP];
  uint32_t t_ms_[CAP];
  int head_ = 0, count_ = 0;

  bool  have_disp_ = false;
  float disp_c_ = 0;
  float rate_c_min_ = 0;

  void recomputeRate();
};
