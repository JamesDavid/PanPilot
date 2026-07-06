// foodlib.cpp — see foodlib.h. Merges the seed table with a user FoodStore:
// a custom entry with the same (name,variant) shadows the seed entry in place;
// custom entries with a new (name,variant) are appended.
#include "core/foodlib.h"
#include "core/foodstore.h"
#include <cstring>

namespace {
const FoodStore* s_custom = nullptr;
const FoodEntry* s_merged[FOODLIB_SEED_COUNT + FoodStore::MAX];
int s_mergedN = 0;

bool same(const FoodEntry& a, const FoodEntry& b) {
  return std::strcmp(a.name, b.name) == 0 &&
         std::strcmp(a.variant, b.variant) == 0;
}

void rebuild() {
  s_mergedN = 0;
  for (int i = 0; i < (int)FOODLIB_SEED_COUNT; ++i) {
    const FoodEntry* c =
        s_custom ? s_custom->find(FOODLIB_SEED[i].name, FOODLIB_SEED[i].variant)
                 : nullptr;
    s_merged[s_mergedN++] = c ? c : &FOODLIB_SEED[i];
  }
  if (s_custom) {
    for (int j = 0; j < s_custom->count(); ++j) {
      const FoodEntry& ce = s_custom->entry(j);
      bool isOverride = false;
      for (int i = 0; i < (int)FOODLIB_SEED_COUNT; ++i)
        if (same(FOODLIB_SEED[i], ce)) { isOverride = true; break; }
      if (!isOverride) s_merged[s_mergedN++] = &ce;
    }
  }
}
}  // namespace

void foodlib_set_custom(const FoodStore* store) { s_custom = store; rebuild(); }

int foodlib_count() {
  return s_custom ? s_mergedN : (int)FOODLIB_SEED_COUNT;
}

const FoodEntry& foodlib_entry(int i) {
  const int n = foodlib_count();
  if (i < 0) i = 0; else if (i >= n) i = n - 1;
  return s_custom ? *s_merged[i] : FOODLIB_SEED[i];
}

const FoodEntry* foodlib_find(const char* name, const char* variant) {
  const int n = foodlib_count();
  for (int i = 0; i < n; ++i) {
    const FoodEntry& e = foodlib_entry(i);
    if (std::strcmp(e.name, name) == 0 && std::strcmp(e.variant, variant) == 0)
      return &e;
  }
  return nullptr;
}
