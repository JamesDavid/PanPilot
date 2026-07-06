// actuator.h — HeatActuator interface (roadmap §3.2). The output stage of the
// control loop. In ADVISORY configuration the "actuator" is the human (the
// cue/attention layer) and no HeatActuator is armed; in ASSIST configuration a
// real actuator (the SSR box or a watchdog-capable smart plug) is armed through
// the §3.3 ceremony. Hardware-free interface; device implementations live in hal/.
#pragma once
#include <stdint.h>

class HeatActuator {
 public:
  virtual ~HeatActuator() = default;
  virtual void setDuty(float duty01) = 0;   // 0..1 time-proportioned power
  virtual void emergencyOff() = 0;          // immediate, unconditional
  virtual bool isAlive() = 0;               // heartbeat / watchdog ack
  virtual uint32_t minCyclePeriodMs() const = 0;  // SSR 2-5 s, plug 30 s
  virtual const char* name() const = 0;
};
