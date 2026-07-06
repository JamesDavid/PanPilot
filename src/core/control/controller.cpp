// controller.cpp — see controller.h.
#include "controller.h"
#include <algorithm>

void Controller::reset() {
  integ_ = 0; have_prev_ = false; prev_temp_ = 0; last_duty_ = 0;
}

float Controller::update(float tempF, float setpointF, float dt_s) {
  if (dt_s <= 0) return last_duty_;

  if (law_ == ControlLaw::BANGBANG) {
    if (tempF < setpointF - band_) last_duty_ = 1.0f;
    else if (tempF > setpointF + band_) last_duty_ = 0.0f;
    // within the band: hold last state (hysteresis)
    return last_duty_;
  }

  // PID with anti-windup + derivative on measurement.
  const float err = setpointF - tempF;
  const float deriv = have_prev_ ? -(tempF - prev_temp_) / dt_s : 0.0f;
  prev_temp_ = tempF; have_prev_ = true;

  float out = kp_ * err + ki_ * integ_ + kd_ * deriv;
  const bool saturated_hi = out > 1.0f && err > 0;
  const bool saturated_lo = out < 0.0f && err < 0;
  if (!saturated_hi && !saturated_lo) integ_ += err * dt_s;   // conditional integ.
  out = kp_ * err + ki_ * integ_ + kd_ * deriv;

  out = std::max(0.0f, std::min(1.0f, out));
  last_duty_ = out;
  return out;
}
