// test_foodstore — custom foods store + the seed merge: appends new foods,
// shadows a seed entry when (name,variant) matches, rejects bad input, and
// reverts cleanly (spec §2.7).
#include <unity.h>
#include "core/foodlib.cpp"
#include "core/foodstore.h"
#include <cstring>
#include <cstdio>

void setUp() { foodlib_set_custom(nullptr); }
void tearDown() { foodlib_set_custom(nullptr); }

static const uint16_t SIDES2[2] = {80, 40};

static void test_seed_only_by_default(void) {
  TEST_ASSERT_EQUAL_INT((int)FOODLIB_SEED_COUNT, foodlib_count());
}

static void test_append_new_food(void) {
  FoodStore s;
  TEST_ASSERT_TRUE(s.add("Halloumi", "1/2-inch slabs", 375, 425, SIDES2, 2,
                         400, -10, 0, 0, "flip when golden"));
  foodlib_set_custom(&s);
  TEST_ASSERT_EQUAL_INT((int)FOODLIB_SEED_COUNT + 1, foodlib_count());
  const FoodEntry* f = foodlib_find("Halloumi", "1/2-inch slabs");
  TEST_ASSERT_NOT_NULL(f);
  TEST_ASSERT_EQUAL_INT(375, f->panTargetF_lo);
  TEST_ASSERT_EQUAL_INT(2, f->sides);
}

static void test_override_shadows_seed(void) {
  // "Pancakes" / "4-inch, standard batter" exists in the seed.
  FoodStore s;
  const uint16_t sides[2] = {200, 100};
  TEST_ASSERT_TRUE(s.add("Pancakes", "4-inch, standard batter", 350, 375, sides,
                         2, 365, -12, 0, 0, "my griddle runs cool"));
  foodlib_set_custom(&s);
  // No new row — it shadowed the seed in place.
  TEST_ASSERT_EQUAL_INT((int)FOODLIB_SEED_COUNT, foodlib_count());
  const FoodEntry* f = foodlib_find("Pancakes", "4-inch, standard batter");
  TEST_ASSERT_NOT_NULL(f);
  TEST_ASSERT_EQUAL_INT(200, f->sideSec[0]);   // custom time, not the seed's 150
  TEST_ASSERT_EQUAL_STRING("my griddle runs cool", f->flipHint);
}

static void test_rejects_bad_input(void) {
  FoodStore s;
  TEST_ASSERT_FALSE(s.add("Bad", "band", 400, 400, SIDES2, 2, 400, 0, 0, 0, ""));
  TEST_ASSERT_FALSE(s.add("Bad", "sides", 300, 350, SIDES2, 0, 400, 0, 0, 0, ""));
  TEST_ASSERT_FALSE(s.add("Bad", "sides", 300, 350, SIDES2, 9, 400, 0, 0, 0, ""));
  TEST_ASSERT_EQUAL_INT(0, s.count());
}

static void test_safe_internal_floor(void) {
  // SAFETY: an override matching a seed (name,variant) can never lower the
  // seed's USDA minimum; a new food keeps whatever was requested.
  TEST_ASSERT_EQUAL_UINT16(165, FoodStore::safeInternalFloor(
      "Chicken breast", "butterflied / ~1/2-inch", 0));      // omitted -> seed
  TEST_ASSERT_EQUAL_UINT16(165, FoodStore::safeInternalFloor(
      "Chicken breast", "butterflied / ~1/2-inch", 150));    // lowered -> seed
  TEST_ASSERT_EQUAL_UINT16(180, FoodStore::safeInternalFloor(
      "Chicken breast", "butterflied / ~1/2-inch", 180));    // raised -> kept
  TEST_ASSERT_EQUAL_UINT16(0, FoodStore::safeInternalFloor(
      "Halloumi", "1/2-inch slabs", 0));                     // new food -> as-is
}

static void test_capacity_and_revert(void) {
  FoodStore s;
  for (int i = 0; i < FoodStore::MAX + 3; ++i) {
    char v[16];
    std::snprintf(v, sizeof(v), "v%d", i);
    s.add("Food", v, 300, 350, SIDES2, 2, 325, 0, 0, 0, "");
  }
  TEST_ASSERT_EQUAL_INT(FoodStore::MAX, s.count());
  foodlib_set_custom(&s);
  TEST_ASSERT_EQUAL_INT((int)FOODLIB_SEED_COUNT + FoodStore::MAX, foodlib_count());
  foodlib_set_custom(nullptr);
  TEST_ASSERT_EQUAL_INT((int)FOODLIB_SEED_COUNT, foodlib_count());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_seed_only_by_default);
  RUN_TEST(test_append_new_food);
  RUN_TEST(test_override_shadows_seed);
  RUN_TEST(test_rejects_bad_input);
  RUN_TEST(test_safe_internal_floor);
  RUN_TEST(test_capacity_and_revert);
  return UNITY_END();
}
