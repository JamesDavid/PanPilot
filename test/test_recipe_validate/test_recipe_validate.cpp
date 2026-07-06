// test_recipe_validate — the §4.1.1 validation rules.
#include <unity.h>
#include "core/recipe_validate.cpp"
#include "core/preplib.h"
#include <cstring>

namespace {
int butterId() {
  for (int i = 0; i < preplib_count(); ++i)
    if (strcmp(preplib_entry(i).name, "Butter") == 0) return i;
  return 0;
}
}  // namespace

void setUp() {}
void tearDown() {}

static void test_valid_program_passes(void) {
  const RecipeStep ok[] = {
    { StepType::HOLD, 450, 0, EXP_NONE, "Preheat" },
    { StepType::CUE, 0, 120, EXP_FOOD_ADDED, "Add patties" },
    { StepType::TIMER, 90, 0, EXP_NONE, "Sear" },
    { StepType::LOOP, 1, 3, EXP_NONE, "" },
    { StepType::END, 0, 0, EXP_NONE, "" },
  };
  TEST_ASSERT_TRUE(recipe_validate(ok, 5).ok);
}

static void test_rejects_700_hold(void) {
  const RecipeStep bad[] = { { StepType::HOLD, 700, 0, EXP_NONE, "too hot" } };
  RecipeVerdict v = recipe_validate(bad, 1);
  TEST_ASSERT_FALSE(v.ok);
  TEST_ASSERT_EQUAL_INT(0, v.badStep);
}

static void test_rejects_butter_then_sear(void) {
  const RecipeStep bad[] = {
    { StepType::PREP, 0, 0, EXP_NONE, "" },        // a=butter set below
    { StepType::HOLD, 500, 0, EXP_NONE, "sear" },  // 500 > butter smoke - 25
  };
  RecipeStep prog[2] = { bad[0], bad[1] };
  prog[0].a = butterId();
  RecipeVerdict v = recipe_validate(prog, 2);
  TEST_ASSERT_FALSE(v.ok);
  TEST_ASSERT_EQUAL_INT(1, v.badStep);
  // ...unless the author acknowledges browning on purpose.
  TEST_ASSERT_TRUE(recipe_validate(prog, 2, /*brownOnPurpose*/ true).ok);
}

static void test_rejects_bad_loop_and_hanging_cue(void) {
  const RecipeStep loop[] = {
    { StepType::HOLD, 400, 0, EXP_NONE, "" },
    { StepType::LOOP, 5, 2, EXP_NONE, "" },        // target 5 not earlier
  };
  TEST_ASSERT_FALSE(recipe_validate(loop, 2).ok);
  const RecipeStep cue[] = { { StepType::CUE, 0, 0, EXP_NONE, "hangs" } };
  TEST_ASSERT_FALSE(recipe_validate(cue, 1).ok);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_valid_program_passes);
  RUN_TEST(test_rejects_700_hold);
  RUN_TEST(test_rejects_butter_then_sear);
  RUN_TEST(test_rejects_bad_loop_and_hanging_cue);
  return UNITY_END();
}
