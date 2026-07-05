// thermal_model.cpp — see thermal_model.h.
#include "thermal_model.h"

void ThermalModel::reset() {
  head_ = 0; count_ = 0; have_disp_ = false; disp_c_ = 0; rate_c_min_ = 0;
}

void ThermalModel::update(const PanReading& r) {
  if (r.presence == PanPresence::ABSENT ||
      r.presence == PanPresence::OBSTRUCTED)
    return;  // don't corrupt the trace when there's no pan to trust

  // Exponential smoothing (τ≈2 s at 4 Hz, base spec §7.1).
  if (!have_disp_) { disp_c_ = r.panTempC; have_disp_ = true; }
  else disp_c_ += SMOOTH_ALPHA * (r.panTempC - disp_c_);

  // Ring buffer of raw samples for slope fitting.
  const int i = (head_ + count_) % CAP;
  if (count_ < CAP) { t_c_[i] = r.panTempC; t_ms_[i] = r.t_ms; ++count_; }
  else {
    t_c_[head_] = r.panTempC; t_ms_[head_] = r.t_ms;
    head_ = (head_ + 1) % CAP;
  }
  recomputeRate();
}

void ThermalModel::recomputeRate() {
  if (count_ < 2) { rate_c_min_ = 0; return; }
  const uint32_t now = t_ms_[(head_ + count_ - 1) % CAP];

  // Least-squares slope over the last RATE_WINDOW_MS (base spec §7.1).
  double sx = 0, sy = 0, sxx = 0, sxy = 0;
  int n = 0;
  uint32_t oldest = now;
  for (int k = 0; k < count_; ++k) {
    const int idx = (head_ + k) % CAP;
    if (now - t_ms_[idx] > (uint32_t)RATE_WINDOW_MS) continue;
    const double x = (double)t_ms_[idx];  // ms
    const double y = t_c_[idx];
    sx += x; sy += y; sxx += x * x; sxy += x * y; ++n;
    if (t_ms_[idx] < oldest) oldest = t_ms_[idx];
  }
  if (n < 2 || (now - oldest) < (uint32_t)RATE_MIN_SPAN_MS) {
    rate_c_min_ = 0;  // not enough span yet -> "estimating" (§7.1)
    return;
  }
  const double denom = (n * sxx - sx * sx);
  if (denom == 0) { rate_c_min_ = 0; return; }
  const double slope_c_per_ms = (n * sxy - sx * sy) / denom;
  rate_c_min_ = (float)(slope_c_per_ms * 60000.0);  // °C per minute
}

Trend ThermalModel::trend() const {
  const float f = rateFPerMin();
  if (f <= -TREND_STABLE_FMIN) return Trend::COOLING;
  if (f < TREND_STABLE_FMIN) return Trend::STABLE;
  if (f < TREND_SLOW_FMIN) return Trend::RISING_SLOW;
  return Trend::RISING_FAST;
}
