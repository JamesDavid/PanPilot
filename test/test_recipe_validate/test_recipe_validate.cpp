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

static void test_rejects_nested_loops(void) {
  // The engine has one loop counter; a LOOP inside another LOOP's body never
  // terminates, so validation must reject it before it can be saved.
  const RecipeStep nested[] = {
    { StepType::HOLD, 400, 0, EXP_NONE, "" },      // 0
    { StepType::TIMER, 10, 0, EXP_NONE, "" },      // 1
    { StepType::LOOP, 1, 2, EXP_NONE, "" },        // 2: inner (body 1..1)
    { StepType::LOOP, 0, 2, EXP_NONE, "" },        // 3: outer body contains 2
    { StepType::END, 0, 0, EXP_NONE, "" },
  };
  RecipeVerdict v = recipe_validate(nested, 5);
  TEST_ASSERT_FALSE(v.ok);
  TEST_ASSERT_EQUAL_INT(3, v.badStep);
  // Two sequential (non-overlapping) loops remain legal.
  const RecipeStep seq[] = {
    { StepType::TIMER, 10, 0, EXP_NONE, "" },      // 0
    { StepType::LOOP, 0, 2, EXP_NONE, "" },        // 1: body 0..0
    { StepType::TIMER, 10, 0, EXP_NONE, "" },      // 2
    { StepType::LOOP, 2, 2, EXP_NONE, "" },        // 3: body 2..2
    { StepType::END, 0, 0, EXP_NONE, "" },
  };
  TEST_ASSERT_TRUE(recipe_validate(seq, 5).ok);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_valid_program_passes);
  RUN_TEST(test_rejects_700_hold);
  RUN_TEST(test_rejects_butter_then_sear);
  RUN_TEST(test_rejects_bad_loop_and_hanging_cue);
  RUN_TEST(test_rejects_nested_loops);
  return UNITY_END();
}
