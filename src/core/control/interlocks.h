// interlocks.h — the safety interlock specification (roadmap §3.3). "Control
// authority is a privilege the perception system must continuously earn." Every
// rule fails safe to power-off. The evaluator runs BEFORE the PID each tick and
// can only lower duty; the PID can never override an interlock. Hardware-free +
// unit-tested (full S1–S11 truth table) — these tests must be green before any
// control code runs on hardware.
#pragma once
#include <stdint.h>
#include "pan_types.h"
#include "app_config.h"

enum class Interlock : uint8_t {
  NONE, S1_CONFIDENCE, S2_PRESENCE, S3_OBSTRUCT, S4_RUNAWAY, S5_MAXTEMP,
  S6_SENSOR, S7_ACTUATOR, S8_COMMS, S9_STOP, S10_UNATTENDED, S11_DIEHEAT
};

struct InterlockInput {
  uint8_t confidence = 100;
  PanPresence presence = PanPresence::PRESENT;
  float duty = 0;                 // currently commanded duty 0..1
  float rateFPerMin = 0;
  float tempF = 70;
  float warnF = 450;
  bool sensorFault = false;
  uint32_t lastFrameMs = 0;       // timestamp of last good frame
  bool actuatorAlive = true;
  uint32_t lastAckMs = 0;         // last actuator ack timestamp
  bool commsOk = true;
  bool stopPressed = false;
  uint32_t lastInteractionMs = 0; // last user interaction / food event
  float dieTempC = 30;
  uint32_t now = 0;
};

class InterlockMonitor {
 public:
  // Returns the first tripped interlock (NONE if control may proceed). Stateful
  // (dwell timers for S1/S3/S4). Any non-NONE result means duty -> 0.
  Interlock evaluate(const InterlockInput& in);
  void reset();
  // S4/S5/S7 are emergency-off + repeating alarm; the rest pause with an alert.
  static bool isEmergency(Interlock i);

 private:
  uint32_t s1_since_ = 0;   // 0 = not currently below threshold
  uint32_t s3_since_ = 0;
  uint32_t s4_since_ = 0;
};
