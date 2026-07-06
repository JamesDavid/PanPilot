// foodlib.h — accessor over the compiled-in seed food database (roadmap §2.7 /
// Appendix A), merged with user custom foods/overrides from a FoodStore. The
// seed table + FoodEntry struct are in foodlib/foodlib_seed.h, dropped in
// verbatim. Until a store is registered these return the seed unchanged.
#pragma once
#include "core/foodlib/foodlib_seed.h"

class FoodStore;

int foodlib_count();
const FoodEntry& foodlib_entry(int i);
const FoodEntry* foodlib_find(const char* name, const char* variant);

// Register the custom store (device: after parsing /foods.json) and rebuild the
// merged view. Passing nullptr reverts to the seed-only list.
void foodlib_set_custom(const FoodStore* store);
