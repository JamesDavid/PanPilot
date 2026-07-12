// foodlib.cpp — see foodlib.h. Merged view = Appendix A seed (verbatim) +
// FOODLIB_USER additions (user-requested, foodlib_user.h) + a user FoodStore:
// a custom entry with the same (name,variant) shadows the compiled entry in
// place; custom entries with a new (name,variant) are appended.
#include "core/foodlib.h"
#include "core/foodlib/foodlib_user.h"
#include "core/foodstore.h"
#include <cstring>

namespace {
const FoodStore* s_custom = nullptr;
const FoodEntry*
    s_merged[FOODLIB_SEED_COUNT + FOODLIB_USER_COUNT + FoodStore::MAX];
int s_mergedN = 0;
bool s_built = false;

bool same(const FoodEntry& a, const FoodEntry& b) {
  return std::strcmp(a.name, b.name) == 0 &&
         std::strcmp(a.variant, b.variant) == 0;
}

// The two compiled tables, viewed as one list.
constexpr int kCompiled = (int)FOODLIB_SEED_COUNT + (int)FOODLIB_USER_COUNT;
const FoodEntry& compiled(int i) {
  return i < (int)FOODLIB_SEED_COUNT
             ? FOODLIB_SEED[i]
             : FOODLIB_USER[i - (int)FOODLIB_SEED_COUNT];
}

void rebuild() {
  s_mergedN = 0;
  for (int i = 0; i < kCompiled; ++i) {
    const FoodEntry& e = compiled(i);
    const FoodEntry* c = s_custom ? s_custom->find(e.name, e.variant) : nullptr;
    s_merged[s_mergedN++] = c ? c : &e;
  }
  if (s_custom) {
    for (int j = 0; j < s_custom->count(); ++j) {
      const FoodEntry& ce = s_custom->entry(j);
      bool isOverride = false;
      for (int i = 0; i < kCompiled; ++i)
        if (same(compiled(i), ce)) { isOverride = true; break; }
      if (!isOverride) s_merged[s_mergedN++] = &ce;
    }
  }
  s_built = true;
}
}  // namespace

void foodlib_set_custom(const FoodStore* store) { s_custom = store; rebuild(); }

int foodlib_count() {
  if (!s_built) rebuild();
  return s_mergedN;
}

const FoodEntry& foodlib_entry(int i) {
  const int n = foodlib_count();
  if (i < 0) i = 0; else if (i >= n) i = n - 1;
  return *s_merged[i];
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
