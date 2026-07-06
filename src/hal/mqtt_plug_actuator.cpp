// mqtt_plug_actuator.cpp — see mqtt_plug_actuator.h.
#if !defined(PANPILOT_SIM)
#include "mqtt_plug_actuator.h"
#include <Arduino.h>
#include <algorithm>

namespace hal {

void MqttPlugActuator::setDuty(float duty01) {
  duty01 = std::max(0.0f, std::min(1.0f, duty01));
  char pct[8];
  snprintf(pct, sizeof(pct), "%d", (int)(duty01 * 100 + 0.5f));
  if (pub_) pub_(topic_, pct);
}

void MqttPlugActuator::emergencyOff() {
  if (pub_) pub_(topic_, "0");
}

}  // namespace hal
#endif
