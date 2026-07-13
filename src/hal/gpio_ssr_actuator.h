// gpio_ssr_actuator.h — HeatActuator on a LOCAL GPIO (bench decision
// 2026-07-12: James prefers a directly-wired SSR on the UART1-OUT header's
// IO17 over the networked SSR box — no broker/Wi-Fi in the control path).
//
// SAFETY MODEL (§3.3 + M14 gates; the roadmap's SSR box has an INDEPENDENT
// hardware watchdog — a bare GPIO does not, so this implementation layers):
//  1. All S1–S11 interlocks run before the PID exactly as for the box.
//  2. Time-proportioning window driven from the control tick; the frame-loss
//     failsafe (main.cpp) forces duty 0 when the sensor stops delivering.
//  3. A DEADMAN esp_timer (its own high-priority dispatch task) drops the pin
//     LOW if setDuty() hasn't been refreshed within SSR_DEADMAN_MS — covers a
//     wedged control task while the RTOS still runs.
//  4. Any crash/reboot leaves the pin Hi-Z -> the REQUIRED external pulldown
//     (10k, SSR input- to GND — HARDWARE_TEST) holds the burner OFF.
//  5. Compiled ONLY when SSR_GPIO_PIN is defined (env:crowpanel35_advance_ssr)
//     — normal builds never touch the pin. DEV_FAKE_SENSOR still refuses to
//     arm, and arming requires live real frames.
// Residual risk: a live-but-wrong control loop that keeps refreshing a high
// duty on purpose. That is what the interlocks + supervised-bench-only rule
// are for; do NOT walk away from a burner on this path until M16/M17 pass.
#pragma once
#if !defined(PANPILOT_SIM) && defined(SSR_GPIO_PIN)
#include "core/control/actuator.h"

namespace hal {

class GpioSsrActuator : public HeatActuator {
 public:
  explicit GpioSsrActuator(uint32_t windowMs) : window_(windowMs) {}

  void begin();   // pin LOW + start the deadman timer; call early in setup()

  void setDuty(float duty01) override;   // control tick: window + refresh deadman
  void emergencyOff() override;          // immediate LOW; deadman left stale
  bool isAlive() override { return true; }   // local pin: no comms to lose
  uint32_t minCyclePeriodMs() const override { return window_; }
  const char* name() const override { return "SSR direct"; }

 private:
  uint32_t window_;
};

}  // namespace hal
#endif
