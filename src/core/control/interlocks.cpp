// interlocks.cpp — see interlocks.h.
#include "interlocks.h"

void InterlockMonitor::reset() { s1_since_ = s3_since_ = s4_since_ = 0; }

bool InterlockMonitor::isEmergency(Interlock i) {
  return i == Interlock::S4_RUNAWAY || i == Interlock::S5_MAXTEMP ||
         i == Interlock::S7_ACTUATOR;
}

Interlock InterlockMonitor::evaluate(const InterlockInput& in) {
  const uint32_t now = in.now;

  // S9 human override — highest priority.
  if (in.stopPressed) return Interlock::S9_STOP;

  // S5 hard max temp — immediate.
  if (in.tempF >= in.warnF + IL_HARD_MAX_OVER_F || in.tempF >= IL_HARD_MAX_ABS_F)
    return Interlock::S5_MAXTEMP;

  // S6 sensor fault / frame gap.
  if (in.sensorFault || (now - in.lastFrameMs) > IL_FRAME_GAP_MS)
    return Interlock::S6_SENSOR;

  // S2 pan presence.
  if (in.presence == PanPresence::ABSENT) return Interlock::S2_PRESENCE;

  // S3 obstruction dwell.
  if (in.presence == PanPresence::OBSTRUCTED) {
    if (s3_since_ == 0) s3_since_ = now ? now : 1;
    if (now - s3_since_ > IL_OBSTRUCT_MS) return Interlock::S3_OBSTRUCT;
  } else {
    s3_since_ = 0;
  }

  // S7 actuator heartbeat.
  if (!in.actuatorAlive || (now - in.lastAckMs) > IL_ACT_UNACK_MS)
    return Interlock::S7_ACTUATOR;

  // S8 comms loss.
  if (!in.commsOk) return Interlock::S8_COMMS;

  // S11 device self-heat.
  if (in.dieTempC > IL_DIE_MAX_C) return Interlock::S11_DIEHEAT;

  // S1 confidence gate (dwell).
  if (in.confidence < IL_CONF_MIN) {
    if (s1_since_ == 0) s1_since_ = now ? now : 1;
    if (now - s1_since_ > IL_CONF_MS) return Interlock::S1_CONFIDENCE;
  } else {
    s1_since_ = 0;
  }

  // S4 runaway / open-loop (high duty, not heating).
  if (in.duty > IL_RUNAWAY_DUTY && in.rateFPerMin <= 0) {
    if (s4_since_ == 0) s4_since_ = now ? now : 1;
    if (now - s4_since_ > IL_RUNAWAY_MS) return Interlock::S4_RUNAWAY;
  } else {
    s4_since_ = 0;
  }

  // S10 unattended timeout.
  if ((now - in.lastInteractionMs) > IL_UNATTENDED_MS)
    return Interlock::S10_UNATTENDED;

  return Interlock::NONE;
}
