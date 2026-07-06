// preplib.h — accessor over the prep-ingredient table (roadmap §4.1.1 /
// Appendix A.1): fats + liquids with add windows, smoke points, and melt-ready
// criteria. Dropped in verbatim at preplib/preplib_seed.h. Hardware-free.
#pragma once
#include "core/preplib/preplib_seed.h"

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
