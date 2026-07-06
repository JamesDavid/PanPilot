// food_json.h — parse user custom foods from LittleFS /foods.json into a
// FoodStore (spec §2.7). Device-only; the merge with the seed lives in
// core/foodlib. Absent or malformed file -> no custom foods (returns <=0).
#pragma once
#if !defined(PANPILOT_SIM)
#include "core/foodstore.h"

namespace hal {
// Returns the number of custom foods loaded, or -1 if there is no file.
int load_custom_foods(FoodStore& store);
}  // namespace hal
#endif
