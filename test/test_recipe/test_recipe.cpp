// test_recipe — the smash-burger x4 program runs end-to-end through the
// sequencer (4 batches via the loop), and the PREP too-hot guard fires
// (roadmap §4.1, §4.1.1).
#include <unity.h>
#include "core/recipe.cpp"
#include "core/preplib.h"
#include <cstring>

namespace {
uint32_t g_now = 0;
RecipeInput mk(float tempF, bool food = false, bool touch = false) {
  RecipeInput in; in.panTempF = tempF; in.foodAdded = food; in.touch = touch;
  in.now = g_now; return in;
}
}  // namespace

void setUp() { g_now = 0; }
void tearDown() {}

static void test_smashburger_runs_four_batches(void) {
  RecipeEngine e;
  e.start(recipe_builtin_smashburger(), 0);
  int batches = 0;
  // Preheat until in band.
  RecipeOut o = e.step(mk(300));
  TEST_ASSERT_EQUAL(int(StepType::HOLD), int(o.type));
  TEST_ASSERT_EQUAL_INT(450, o.setpointF);
  o = e.step(mk(450));                              // reached -> advance to CUE
  TEST_ASSERT_EQUAL(int(StepType::CUE), int(o.type));

  for (int guard = 0; guard < 40 && !o.finished; ++guard) {
    // Drive one full batch: add patties, sear, flip, finish, remove.
    o = e.step(mk(450, /*food*/ true));             // "Add patties" satisfied
    // step into TIMER (searing) — advance time
    g_now += 95000; o = e.step(mk(450));            // side 1 done -> Flip cue
    o = e.step(mk(450, false, /*touch*/ true));     // flip
    g_now += 65000; o = e.step(mk(450));            // finishing done -> Remove cue
    o = e.step(mk(450, false, /*touch*/ true));     // remove -> LOOP
    ++batches;
    g_now += 1000;
    o = e.step(mk(450));                            // resolve loop/next batch or END
    if (o.finished) break;
  }
  TEST_ASSERT_EQUAL_INT(4, batches);                // 1 + 3 loops
  TEST_ASSERT_TRUE(o.finished);
}

static void test_prep_too_hot_for_butter(void) {
  // Find butter's index; a PREP butter step on a pan above maxAddTempF holds.
  int butter = 0;
  for (int i = 0; i < preplib_count(); ++i)
    if (strcmp(preplib_entry(i).name, "Butter") == 0) { butter = i; break; }
  const PrepEntry& b = preplib_entry(butter);

  static const RecipeStep steps[] = {
    { StepType::PREP, 0, 0, EXP_NONE, "" },  // a filled below
    { StepType::END, 0, 0, EXP_NONE, "" },
  };
  RecipeStep s0 = steps[0]; s0.a = butter;
  RecipeStep prog_steps[] = { s0, steps[1] };
  RecipeProgram prog = { "prep", prog_steps, 2 };

  RecipeEngine e; e.start(&prog, 0);
  RecipeOut o = e.step(mk(b.maxAddTempF + 50));     // pan too hot
  TEST_ASSERT_TRUE(o.prepTooHot);
  TEST_ASSERT_EQUAL_INT(b.addTempF_hi, o.setpointF);   // cools to add window
  o = e.step(mk(b.addTempF_lo, false, /*touch*/ true));  // cooled + confirm
  TEST_ASSERT_FALSE(o.prepTooHot);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_smashburger_runs_four_batches);
  RUN_TEST(test_prep_too_hot_for_butter);
  return UNITY_END();
}
