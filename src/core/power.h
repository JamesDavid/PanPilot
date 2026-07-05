// power.h — ACTIVE/IDLE power state machine (base spec §8). Hardware-free +
// unit-tested; main drives backlight + sensor cadence from the state and feeds
// it touch inactivity, scene-hot, and heating-detected signals.
#pragma once
#include <stdint.h>
#include "app_config.h"

enum class PowerState : uint8_t { ACTIVE, IDLE };

class PowerFsm {
 public:
  struct In {
    uint32_t inactiveMs;   // since last touch (LVGL inactivity)
    bool sceneHot;         // any region meaningfully above ambient
    bool heatingDetected;  // rising heat event (§8 wake)
  };

  PowerState step(const In& in);
  PowerState state() const { return state_; }
  bool wokeThisStep() const { return woke_; }
  bool wokeByHeat() const { return woke_by_heat_; }

 private:
  PowerState state_ = PowerState::ACTIVE;
  bool woke_ = false;
  bool woke_by_heat_ = false;
};
