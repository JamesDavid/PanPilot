// gpio_ssr_actuator.cpp — see gpio_ssr_actuator.h.
#if !defined(PANPILOT_SIM)
#include "hal/gpio_ssr_actuator.h"   // self-guards on SSR_GPIO_PIN
#endif
#if !defined(PANPILOT_SIM) && defined(SSR_GPIO_PIN)
#include <Arduino.h>
#include <esp_timer.h>

#include "app_config.h"

namespace hal {
namespace {
volatile uint32_t s_lastRefreshMs = 0;   // last setDuty() from the control tick

// Deadman: runs on esp_timer's own dispatch task, independent of SensorTask —
// if the control loop stops refreshing, the pin drops within one period.
void deadman_cb(void*) {
  if (millis() - s_lastRefreshMs > SSR_DEADMAN_MS)
    digitalWrite(SSR_GPIO_PIN, LOW);
}
}  // namespace

void GpioSsrActuator::begin() {
  pinMode(SSR_GPIO_PIN, OUTPUT);
  digitalWrite(SSR_GPIO_PIN, LOW);
  s_lastRefreshMs = 0;                    // stale until the first control tick
  static esp_timer_handle_t t = nullptr;
  if (!t) {
    const esp_timer_create_args_t args = {
        .callback = deadman_cb,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "ssr_deadman",
        .skip_unhandled_events = true,
    };
    if (esp_timer_create(&args, &t) == ESP_OK)
      esp_timer_start_periodic(t, (uint64_t)SSR_DEADMAN_MS * 1000 / 3);
  }
  Serial.printf("[ssr] direct GPIO actuator on IO%d, window %lu ms, deadman %d ms\n",
                (int)SSR_GPIO_PIN, (unsigned long)window_, (int)SSR_DEADMAN_MS);
}

void GpioSsrActuator::setDuty(float duty01) {
  if (duty01 < 0) duty01 = 0;
  if (duty01 > 1) duty01 = 1;
  s_lastRefreshMs = millis();
  // Time-proportioning against the window: control ticks (~8 Hz) re-evaluate
  // which phase of the window we are in. Granularity ~ tick/window (~4%).
  const uint32_t phase = millis() % window_;
  const bool on = duty01 > 0.01f && phase < (uint32_t)(duty01 * (float)window_);
  digitalWrite(SSR_GPIO_PIN, on ? HIGH : LOW);
}

void GpioSsrActuator::emergencyOff() {
  digitalWrite(SSR_GPIO_PIN, LOW);
  // Deliberately do NOT refresh the deadman: unless the control loop resumes
  // commanding, the periodic check keeps forcing the pin low.
}

}  // namespace hal
#endif
