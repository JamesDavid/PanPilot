// controller.h — the control law (roadmap §3.2): time-proportional bang-bang
// (M16) -> PID (M17). Output is a duty 0..1 fed to a HeatActuator. PID has
// anti-windup and derivative-on-measurement; the plant time constant is minutes,
// so slow PWM is indistinguishable from proportional power (no phase-angle
// control). Hardware-free + unit-tested against a simulated first-order plant.
#pragma once
#include <stdint.h>

enum class ControlLaw : uint8_t { BANGBANG, PID };

class Controller {
 public:
  void setLaw(ControlLaw law) { law_ = law; }
  void setGains(float kp, float ki, float kd) { kp_ = kp; ki_ = ki; kd_ = kd; }
  void setBand(float f) { band_ = f; }          // bang-bang hysteresis (°F)
  void reset();

  // Advance one control tick. dt_s > 0. Returns duty 0..1.
  float update(float tempF, float setpointF, float dt_s);
  float integral() const { return integ_; }

 private:
  ControlLaw law_ = ControlLaw::BANGBANG;
  float kp_ = 0.02f, ki_ = 0.0008f, kd_ = 0.02f;
  float band_ = 3.0f;
  float integ_ = 0;
  float prev_temp_ = 0;
  bool have_prev_ = false;
  float last_duty_ = 0;   // bang-bang state (for hysteresis)
};
