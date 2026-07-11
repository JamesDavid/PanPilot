// guidance.cpp — see guidance.h.
#include "guidance.h"
#include <algorithm>

void Target::setCenter(int c) {
  centerF = c;
  loF = c - TARGET_READY_WINDOW_F;
  hiF = c + TARGET_READY_WINDOW_F;
  warnF = std::min((float)c + TARGET_WARN_OVER_F, ABS_MAX_TEMP_F);
}

void GuidanceEngine::reset() {
  state_ = pending_ = GuidanceState::IDLE;
  pending_ticks_ = 0; away_ticks_ = 0; ready_armed_ = true; ever_ready_ = false;
  last_toohot_ms_ = 0;
}

GuidanceState GuidanceEngine::classify(const GuidanceInput& in, const Target& t,
                                       float projPeak) const {
  if (in.presence == PanPresence::ABSENT) return GuidanceState::NO_PAN;
  if (in.moved) return GuidanceState::CHECK_AIM;
  if (in.presence == PanPresence::UNCERTAIN ||
      in.confidence < CONFIDENCE_UNCERTAIN)
    return GuidanceState::LOW_CONFIDENCE;

  const float T = in.tempF, rate = in.rateFPerMin;
  if (T >= t.warnF || T >= ABS_MAX_TEMP_F) {
    // Bare stainless is reflective and reads erratically; a "too hot" below
    // 300 °F is almost always a misread — trust the trend instead (§7.5).
    if (in.stainlessHint && T < STAINLESS_TOOHOT_MIN_F && T < ABS_MAX_TEMP_F)
      return GuidanceState::LOW_CONFIDENCE;
    return GuidanceState::TOO_HOT;
  }
  if (T > t.hiF) return GuidanceState::TURN_DOWN_NOW;
  if (T >= t.loF) return GuidanceState::READY;

  // Below the target band.
  if (ever_ready_ && rate < -TREND_STABLE_FMIN) return GuidanceState::COOLING;
  if (projPeak > t.hiF) return GuidanceState::TURN_DOWN_NOW;   // overshoot!
  if ((t.loF - T) <= TURN_DOWN_SOON_BAND_F && rate >= RATE_MEDIUM_FMIN)
    return GuidanceState::TURN_DOWN_SOON;
  if (rate > TREND_STABLE_FMIN) return GuidanceState::HOLD;    // rising, ok
  return GuidanceState::HEAT_MORE;
}

GuidanceOutput GuidanceEngine::step(const GuidanceInput& in, const Target& t,
                                    uint32_t now_ms) {
  GuidanceOutput out;
  out.projectedPeakF = in.tempF + in.rateFPerMin * in.lagMinutes;

  // ETA to reach the ready band (base spec §7.1).
  if (in.tempF < t.loF && in.rateFPerMin >= ETA_MIN_RATE_FMIN &&
      in.confidence >= ETA_MIN_CONFIDENCE) {
    int sec = (int)((t.loF - in.tempF) / in.rateFPerMin * 60.0f);
    out.etaSeconds = std::min(sec, ETA_CLAMP_SEC);
  } else {
    out.etaSeconds = -1;
  }

  // Classify + hysteresis (2 consecutive ticks to change, §7.2).
  const GuidanceState raw = classify(in, t, out.projectedPeakF);
  if (raw != state_) {
    if (raw == pending_) ++pending_ticks_;
    else { pending_ = raw; pending_ticks_ = 1; }
    // Starvation guard: an input ALTERNATING between two non-current states
    // (e.g. TOO_HOT / LOW_CONFIDENCE on flickering stainless) resets the
    // pending count every tick and would freeze state_ forever — while the
    // pan is genuinely over the warn threshold and no alarm fires. After
    // 2x the window away from state_, adopt the latest classification.
    ++away_ticks_;
    if (pending_ticks_ >= GUIDANCE_HYSTERESIS_TICKS ||
        away_ticks_ >= 2 * GUIDANCE_HYSTERESIS_TICKS) {
      state_ = raw;
      away_ticks_ = 0;
    }
  } else {
    pending_ = raw; pending_ticks_ = 0; away_ticks_ = 0;
  }

  // Re-arm the READY chime once we leave the band by the hysteresis margin.
  if (in.tempF < t.loF - READY_REARM_F || in.tempF > t.hiF + READY_REARM_F)
    ready_armed_ = true;

  // Alerts on state edges (base spec §7.2).
  const bool entered = (state_ != prev_state_);
  out.alert = AlertAction::NONE;
  if (state_ == GuidanceState::READY) {
    ever_ready_ = true;
    // Chime on ENTRY only (edge), gated by the re-arm flag — so re-arming while
    // hysteresis still holds state_ in READY can't fire a spurious chime.
    if (entered && ready_armed_) {
      out.alert = AlertAction::READY_CHIME; ready_armed_ = false;
    }
  } else if (state_ == GuidanceState::TOO_HOT) {
    if (last_toohot_ms_ == 0 || now_ms - last_toohot_ms_ >= TOO_HOT_REPEAT_MS) {
      out.alert = AlertAction::TOO_HOT_ALARM;
      last_toohot_ms_ = now_ms;
    }
  } else if (state_ == GuidanceState::TURN_DOWN_NOW && entered) {
    out.alert = AlertAction::TURN_DOWN;   // once on entry
  }

  prev_state_ = state_;
  out.state = state_;
  return out;
}
