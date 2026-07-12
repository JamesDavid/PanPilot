// burnermap.cpp — see burnermap.h.
#include "burnermap.h"

namespace {
const char* kNames[BURNER_SETTINGS] = {"LOW", "MED-LOW", "MED", "MED-HIGH", "HIGH"};
}

const char* burner_setting_name(int i) {
  if (i < 0) i = 0;
  if (i >= BURNER_SETTINGS) i = BURNER_SETTINGS - 1;
  return kNames[i];
}

float burnermap_kloss(const BurnerMap& m) {
  const float dT = m.coolTempF - m.ambientF;
  if (dT < 20.0f) return 0;              // cooled too close to ambient to trust
  return -m.coolRateF / dT;              // coolRate is negative -> kLoss > 0
}

float burnermap_settleF(const BurnerMap& m, int setting) {
  if (setting < 0 || setting >= BURNER_SETTINGS) return 0;
  const float kLoss = burnermap_kloss(m);
  if (kLoss <= 1e-4f) return 0;
  // P(s) = rate_s + kLoss*(measTemp_s - ambient);  Tss = ambient + P/kLoss
  const float P = m.rateF[setting] + kLoss * (m.measTempF[setting] - m.ambientF);
  return m.ambientF + P / kLoss;
}

int burnermap_suggest(const BurnerMap& m, float targetF) {
  if (!m.valid) return -1;
  int best = -1;
  float bestD = 1e9f;
  for (int s = 0; s < BURNER_SETTINGS; ++s) {
    const float ss = burnermap_settleF(m, s);
    if (ss <= 0) continue;
    const float d = ss > targetF ? ss - targetF : targetF - ss;
    if (d < bestD) { bestD = d; best = s; }
  }
  return best;
}

const char* burnermap_suggest_name(const BurnerMap& m, float targetF) {
  const int s = burnermap_suggest(m, targetF);
  return s >= 0 ? kNames[s] : nullptr;
}

// ---------------------------------------------------------------------------

void BurnerMapper::start(float ambientF, uint32_t now) {
  map_ = BurnerMap{};
  map_.ambientF = ambientF;
  idx_ = 0;
  phase_ = AWAIT_KNOB;
  t0_ = now;
}

void BurnerMapper::cancel() { phase_ = IDLE; }

void BurnerMapper::confirm(uint32_t now) {
  if (phase_ == AWAIT_KNOB) { phase_ = SETTLING; t0_ = now; }
  else if (phase_ == AWAIT_OFF) { phase_ = COOL_SETTLING; t0_ = now; }
}

int BurnerMapper::secondsLeft(uint32_t now) const {
  uint32_t total = 0;
  switch (phase_) {
    case SETTLING:       total = SETTLE_MS; break;
    case MEASURING:      total = MEASURE_MS; break;
    case COOL_SETTLING:  total = COOL_SETTLE_MS; break;
    case COOL_MEASURING: total = COOL_MEASURE_MS; break;
    default: return 0;
  }
  const uint32_t el = now - t0_;
  return el >= total ? 0 : (int)((total - el + 999) / 1000);
}

void BurnerMapper::update(float tempF, uint32_t now) {
  switch (phase_) {
    case SETTLING:
      if (now - t0_ >= SETTLE_MS) { phase_ = MEASURING; t0_ = now; startT_ = tempF; }
      break;
    case MEASURING:
      if (now - t0_ >= MEASURE_MS) {
        map_.rateF[idx_] = (tempF - startT_) * 60000.0f / (float)(now - t0_);
        map_.measTempF[idx_] = (startT_ + tempF) * 0.5f;
        ++idx_;
        if (idx_ >= BURNER_SETTINGS) phase_ = AWAIT_OFF;
        else { phase_ = AWAIT_KNOB; }
      }
      break;
    case COOL_SETTLING:
      if (now - t0_ >= COOL_SETTLE_MS) { phase_ = COOL_MEASURING; t0_ = now; startT_ = tempF; }
      break;
    case COOL_MEASURING:
      if (now - t0_ >= COOL_MEASURE_MS) {
        map_.coolRateF = (tempF - startT_) * 60000.0f / (float)(now - t0_);
        map_.coolTempF = (startT_ + tempF) * 0.5f;
        finishIfComplete();
        phase_ = DONE;
      }
      break;
    default:
      break;
  }
}

void BurnerMapper::finishIfComplete() {
  // Usable = a real loss estimate and at least a plausible spread of rates.
  map_.valid = burnermap_kloss(map_) > 1e-4f &&
               map_.rateF[BURNER_SETTINGS - 1] > map_.rateF[0];
}
