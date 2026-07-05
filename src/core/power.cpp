// power.cpp — see power.h.
#include "power.h"

PowerState PowerFsm::step(const In& in) {
  woke_ = false;
  woke_by_heat_ = false;
  if (state_ == PowerState::ACTIVE) {
    // Dim only after a long idle AND the scene has gone cool (base spec §8).
    if (in.inactiveMs > IDLE_TIMEOUT_MS && !in.sceneHot)
      state_ = PowerState::IDLE;
  } else {  // IDLE
    const bool touched = in.inactiveMs < WAKE_TOUCH_MS;
    if (touched || in.heatingDetected) {
      state_ = PowerState::ACTIVE;
      woke_ = true;
      woke_by_heat_ = in.heatingDetected && !touched;
    }
  }
  return state_;
}
