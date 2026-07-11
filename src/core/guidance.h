// guidance.h — the guidance state machine (base spec §7.2) + ETA and overshoot
// prediction (§7.1). Hardware-free, unit-tested. Consumes model outputs +
// presence and a Target; emits a GuidanceState, ETA, projected peak, and an
// alert action for the caller to route to the buzzer. The overshoot predictor
// (projected peak -> TURN_DOWN_NOW before the target) is the flagship feature.
#pragma once
#include <stdint.h>
#include "pan_types.h"
#include "app_config.h"

enum class GuidanceState : uint8_t {
  IDLE, NO_PAN, HEAT_MORE, HOLD, TURN_DOWN_SOON, TURN_DOWN_NOW, READY,
  TOO_HOT, COOLING, RECOVERING, CHECK_AIM, LOW_CONFIDENCE
};

enum class AlertAction : uint8_t { NONE, READY_CHIME, TURN_DOWN, TOO_HOT_ALARM };

// Target band in °F (base spec §7.3). lo..hi = ready window; warn = overheat.
struct Target {
  float loF = TARGET_DEFAULT_CENTER_F - TARGET_READY_WINDOW_F;
  float hiF = TARGET_DEFAULT_CENTER_F + TARGET_READY_WINDOW_F;
  float warnF = TARGET_DEFAULT_CENTER_F + TARGET_WARN_OVER_F;
  int centerF = TARGET_DEFAULT_CENTER_F;

  void setCenter(int c);          // recompute lo/hi/warn around a center
};

struct GuidanceInput {
  float tempF = 0;
  float rateFPerMin = 0;
  uint8_t confidence = 0;
  PanPresence presence = PanPresence::UNCERTAIN;
  bool moved = false;            // was uninitialized — zone-2 read it as UB
  bool stainlessHint = false;    // bare stainless read (reflective, unreliable)
  float lagMinutes = LAG_MINUTES_DEFAULT;
};

struct GuidanceOutput {
  GuidanceState state = GuidanceState::IDLE;
  int etaSeconds = -1;            // -1 = unknown/estimating
  float projectedPeakF = 0;
  AlertAction alert = AlertAction::NONE;
};

class GuidanceEngine {
 public:
  GuidanceOutput step(const GuidanceInput& in, const Target& t, uint32_t now_ms);
  void reset();
  GuidanceState state() const { return state_; }

 private:
  GuidanceState state_ = GuidanceState::IDLE;
  GuidanceState prev_state_ = GuidanceState::IDLE;   // for alert edge-detect
  GuidanceState pending_ = GuidanceState::IDLE;
  int pending_ticks_ = 0;
  bool ready_armed_ = true;       // may chime on next READY entry
  bool ever_ready_ = false;
  uint32_t last_toohot_ms_ = 0;

  GuidanceState classify(const GuidanceInput& in, const Target& t,
                         float projPeak) const;
};
