// preplib.h — accessor over the prep-ingredient table (roadmap §4.1.1 /
// Appendix A.1): fats + liquids with add windows, smoke points, and melt-ready
// criteria. Dropped in verbatim at preplib/preplib_seed.h. Hardware-free.
#pragma once
#include <cmath>
#include <stdint.h>
#include "core/preplib/preplib_seed.h"
#include "app_config.h"

inline int preplib_count() { return (int)PREPLIB_SEED_COUNT; }
inline const PrepEntry& preplib_entry(int i) {
  if (i < 0) i = 0; else if (i >= (int)PREPLIB_SEED_COUNT) i = PREPLIB_SEED_COUNT - 1;
  return PREPLIB_SEED[i];
}

// Fat-vs-temperature check (roadmap §4.1.1): would this pan temp burn the fat?
// True = safe (below smoke point − 25 °F). smokePointF==0 (water) is always ok.
inline bool prep_temp_ok(int prepId, float panTempF) {
  const PrepEntry& p = preplib_entry(prepId);
  if (p.smokePointF == 0) return true;
  return panTempF <= (float)p.smokePointF - 25.0f;
}

// Runtime prep/fat monitor (roadmap §4.1.1). Given the pan state during a PREP
// step, tells the caller whether the pan is too hot to add the fat, still too
// cool, or in the add window and READY (melt/equalize/immediate per criterion),
// plus the fat-state overheat clamp to hold while the fat is in the pan.
struct PrepStatus {
  bool tooHot = false;        // above maxAddTempF — burns the fat, cool first
  bool belowWindow = false;   // below the add window — keep heating
  bool ready = false;         // in window AND the ready criterion is met
  int coolToF = 0;            // suggested setpoint when tooHot (add-window top)
  int fatClampWarnF = 0;      // overheat clamp while fat is in pan (0 = none)
  const char* advice = "";    // the fat's ready hint (fat advice)
};

// dwellMsInWindow: how long the pan has continuously been in the add window
// (0 if not currently in it). brownOnPurpose relaxes the clamp to the smoke
// point itself (the program author opted into browning).
inline PrepStatus prep_evaluate(int prepId, float panTempF, float rateFPerMin,
                                uint32_t dwellMsInWindow, bool brownOnPurpose) {
  const PrepEntry& p = preplib_entry(prepId);
  PrepStatus s;
  s.advice = p.readyHint;
  s.fatClampWarnF = (p.smokePointF == 0)
                        ? 0
                        : (int)(brownOnPurpose ? p.smokePointF
                                               : p.smokePointF - 25);
  if (panTempF > (float)p.maxAddTempF) { s.tooHot = true; s.coolToF = p.addTempF_hi; return s; }
  if (panTempF < (float)p.addTempF_lo) { s.belowWindow = true; return s; }
  switch (p.readyCriterion) {
    case READY_IMMEDIATE: s.ready = true; break;
    case READY_MELTED:    s.ready = dwellMsInWindow >= (uint32_t)PREP_MELT_DWELL_MS; break;
    case READY_EQUALIZED: s.ready = std::fabs(rateFPerMin) <= PREP_EQUALIZE_RATE_FMIN; break;
  }
  return s;
}
