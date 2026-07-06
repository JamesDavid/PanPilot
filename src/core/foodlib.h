// foodlib.h — accessor over the compiled-in seed food database (roadmap §2.7 /
// Appendix A). The seed table + FoodEntry struct are in foodlib/foodlib_seed.h,
// dropped in verbatim. Hardware-free; user overrides (LittleFS JSON) shadow
// entries in a later milestone.
#pragma once
#include "core/foodlib/foodlib_seed.h"

inline int foodlib_count() { return (int)FOODLIB_SEED_COUNT; }
inline const FoodEntry& foodlib_entry(int i) {
  if (i < 0) i = 0; else if (i >= (int)FOODLIB_SEED_COUNT) i = FOODLIB_SEED_COUNT - 1;
  return FOODLIB_SEED[i];
}
