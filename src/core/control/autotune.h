// autotune.h — relay (Astrom-Hagglund) PID autotuner (roadmap §3.2). Drives the
// actuator as a relay around the setpoint, measures the induced oscillation's
// amplitude and period, and derives Ziegler-Nichols PID gains from the ultimate
// gain/period. Hardware-free + unit-tested against the same first-order plant as
// the controller; on-device it only runs while ASSIST is armed (bench-gated).
#pragma once
#include <stdint.h>

class RelayAutotuner {
 public:
  // Relay swings duty between dLow..dHigh around setpointF; hysteresis avoids
  // chattering on sensor noise.
  void start(float setpointF, float dLow, float dHigh, float hysteresisF,
             uint32_t now);
  // Advance one tick; returns the relay duty to command. dt is derived from now.
  float update(float tempF, uint32_t now);

  bool running() const { return running_; }
  bool converged() const { return converged_; }
  int cycles() const { return cycles_; }        // completed peaks (progress)
  float kp() const { return kp_; }
  float ki() const { return ki_; }
  float kd() const { return kd_; }
  float ku() const { return ku_; }
  float tuSeconds() const { return tu_; }

  static constexpr int TARGET_CYCLES = 5;        // peaks to observe before solving

 private:
  void solve();

  bool running_ = false, converged_ = false;
  float setpoint_ = 0, dLow_ = 0, dHigh_ = 1, hyst_ = 1.0f;
  float output_ = 0;
  float curMax_ = 0, curMin_ = 0;
  uint32_t lastPeakMs_ = 0;
  bool havePeak_ = false;
  int cycles_ = 0;
  // rolling records of the last few peak-to-peak amplitudes and periods
  static constexpr int N = 4;
  float amp_[N] = {0};
  float per_[N] = {0};   // seconds
  int ampN_ = 0, perN_ = 0;
  float kp_ = 0, ki_ = 0, kd_ = 0, ku_ = 0, tu_ = 0;
};
