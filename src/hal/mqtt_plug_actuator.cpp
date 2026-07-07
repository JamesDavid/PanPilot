// mqtt_plug_actuator.cpp — see mqtt_plug_actuator.h.
#if !defined(PANPILOT_SIM)
#include "mqtt_plug_actuator.h"
#include <Arduino.h>
#include <algorithm>

namespace hal {

// Called from SensorTask (core 0): stage only. PubSubClient is single-threaded
// and owned by the loop core; publishing here raced mqtt.loop() (shared RX/TX
// buffer) and could corrupt the duty command on the wire.
void MqttPlugActuator::setDuty(float duty01) {
  duty01 = std::max(0.0f, std::min(1.0f, duty01));
  pending_pct_ = (int)(duty01 * 100 + 0.5f);
  dirty_ = true;
}

void MqttPlugActuator::emergencyOff() {
  pending_pct_ = 0;
  dirty_ = true;
}

// Loop core only. Clear-then-read: if SensorTask restages between the two, the
// re-set dirty flag just causes one extra publish next iteration.
void MqttPlugActuator::pump() {
  if (!dirty_ || !pub_) return;
  dirty_ = false;
  char pct[8];
  snprintf(pct, sizeof(pct), "%d", (int)pending_pct_);
  pub_(topic_, pct);
}

}  // namespace hal
#endif
