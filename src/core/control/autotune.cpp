// autotune.cpp — see autotune.h.
#include "autotune.h"
#include <cmath>

namespace {
constexpr float PI_F = 3.14159265f;
float avg(const float* v, int n) {
  if (n <= 0) return 0;
  float s = 0;
  for (int i = 0; i < n; ++i) s += v[i];
  return s / n;
}
bool stable(const float* v, int n, float tol) {   // all within tol of the mean
  if (n < 2) return false;
  const float m = avg(v, n);
  if (m == 0) return false;
  for (int i = 0; i < n; ++i)
    if (std::fabs(v[i] - m) > tol * m) return false;
  return true;
}
}  // namespace

void RelayAutotuner::start(float setpointF, float dLow, float dHigh,
                           float hysteresisF, uint32_t now) {
  running_ = true; converged_ = false;
  setpoint_ = setpointF; dLow_ = dLow; dHigh_ = dHigh;
  hyst_ = hysteresisF > 0 ? hysteresisF : 1.0f;
  output_ = dHigh_;                 // kick with heat
  curMax_ = setpointF; curMin_ = setpointF;
  lastPeakMs_ = now; havePeak_ = false;
  cycles_ = 0; ampN_ = 0; perN_ = 0;
  kp_ = ki_ = kd_ = ku_ = tu_ = 0;
}

float RelayAutotuner::update(float tempF, uint32_t now) {
  if (!running_) return 0;

  const bool wasHigh = output_ == dHigh_;
  if (tempF > setpoint_ + hyst_) output_ = dLow_;
  else if (tempF < setpoint_ - hyst_) output_ = dHigh_;

  if (tempF > curMax_) curMax_ = tempF;
  if (tempF < curMin_) curMin_ = tempF;

  const bool isHigh = output_ == dHigh_;
  if (isHigh != wasHigh) {                     // relay switched
    if (!isHigh) {                             // finished a heating half-cycle
      const float amp = curMax_ - curMin_;     // peak-to-peak this cycle
      amp_[ampN_ % N] = amp; ++ampN_;
      if (havePeak_) {
        per_[perN_ % N] = (now - lastPeakMs_) / 1000.0f; ++perN_;
      }
      lastPeakMs_ = now; havePeak_ = true;
      ++cycles_;
      if (cycles_ >= TARGET_CYCLES) solve();
    }
    curMax_ = tempF; curMin_ = tempF;          // reset extrema for next half-cycle
  }
  return output_;
}

void RelayAutotuner::solve() {
  const int an = ampN_ < N ? ampN_ : N;
  const int pn = perN_ < N ? perN_ : N;
  if (an < 2 || pn < 2) return;
  // Require the oscillation to have settled before trusting the numbers.
  if (!stable(amp_, an, 0.20f) || !stable(per_, pn, 0.15f)) return;

  const float a = avg(amp_, an) / 2.0f;        // oscillation amplitude
  const float d = (dHigh_ - dLow_) / 2.0f;     // relay amplitude
  tu_ = avg(per_, pn);
  if (a <= 0 || tu_ <= 0) return;

  ku_ = (4.0f * d) / (PI_F * a);               // ultimate gain
  // Ziegler-Nichols "classic PID": Kp=0.6Ku, Ti=0.5Tu, Td=0.125Tu.
  kp_ = 0.6f * ku_;
  const float ti = 0.5f * tu_;
  const float td = 0.125f * tu_;
  ki_ = ti > 0 ? kp_ / ti : 0;
  kd_ = kp_ * td;
  converged_ = true;
  running_ = false;
}
