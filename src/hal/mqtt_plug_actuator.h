// mqtt_plug_actuator.h — HeatActuator over MQTT (roadmap §3.2, M14). Drives both
// the PanPilot SSR box (min cycle ~2-3 s) and generic watchdog-capable smart
// plugs (min cycle 30 s for relay life). Device-only. Duty is published as a
// percentage; liveness comes from the box's retained status/LWT (feeds S7/S8).
// The box's own hardware watchdog turns it off without our commands, so a comms
// loss is inherently fail-safe. Compile-verified; live drive is bench-tested.
#pragma once
#if !defined(PANPILOT_SIM)
#include "core/control/actuator.h"

namespace hal {

using PublishFn = void (*)(const char* topic, const char* payload);

class MqttPlugActuator : public HeatActuator {
 public:
  MqttPlugActuator(const char* name, uint32_t minCycleMs, PublishFn pub,
                   const char* dutyTopic)
      : name_(name), minCycle_(minCycleMs), pub_(pub), topic_(dutyTopic) {}

  void setDuty(float duty01) override;
  void emergencyOff() override;
  bool isAlive() override { return alive_; }
  uint32_t minCyclePeriodMs() const override { return minCycle_; }
  const char* name() const override { return name_; }

  void setAlive(bool a) { alive_ = a; }   // called when the box status arrives
  void noteAckNow(uint32_t ms) { last_ack_ms_ = ms; }
  uint32_t lastAckMs() const { return last_ack_ms_; }

 private:
  const char* name_;
  uint32_t minCycle_;
  PublishFn pub_;
  const char* topic_;
  bool alive_ = false;
  uint32_t last_ack_ms_ = 0;
};

}  // namespace hal
#endif
