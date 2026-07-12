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

static void test_one_touch_advances_one_gated_step(void) {
  // Two consecutive touch-gated CUEs: a single tap must satisfy only the
  // FIRST — the old engine re-read the same in.touch inside its resolution
  // loop and blew through both.
  static const RecipeStep steps[] = {
    { StepType::CUE, 0, 0, EXP_TOUCH, "first" },
    { StepType::CUE, 0, 0, EXP_TOUCH, "second" },
    { StepType::END, 0, 0, EXP_NONE, "" },
  };
  RecipeProgram prog = { "two-cues", steps, 3 };
  RecipeEngine e; e.start(&prog, 0);
  RecipeOut o = e.step(mk(300, false, /*touch*/ true));   // one tap
  TEST_ASSERT_EQUAL_INT(1, o.stepIndex);                  // sits on "second"
  TEST_ASSERT_FALSE(o.finished);
  o = e.step(mk(300, false, /*touch*/ true));             // second tap
  TEST_ASSERT_TRUE(o.finished);
}

static void test_fat_clamp_survives_prep_advance(void) {
  // SAFETY (§4.1.1): once the butter PREP is in play, the smoke clamp must be
  // carried on every later tick — even when the confirming tap advances PREP
  // within the same step() call (the old code returned the NEXT step's zeroed
  // output, silently dropping the clamp).
  int butter = 0;
  for (int i = 0; i < preplib_count(); ++i)
    if (strcmp(preplib_entry(i).name, "Butter") == 0) { butter = i; break; }
  const PrepEntry& b = preplib_entry(butter);

  RecipeStep steps[] = {
    { StepType::PREP, butter, 0, EXP_NONE, "" },
    { StepType::CUE, 0, 0, EXP_FOOD_ADDED, "add eggs" },   // NOT touch-gated
    { StepType::END, 0, 0, EXP_NONE, "" },
  };
  RecipeProgram prog = { "butter", steps, 3 };
  RecipeEngine e; e.start(&prog, 0);
  // Confirm the butter with a tap while in the add window: PREP advances to
  // the CUE inside this same call...
  RecipeOut o = e.step(mk(b.addTempF_lo + 5, false, /*touch*/ true));
  TEST_ASSERT_EQUAL_INT(1, o.stepIndex);
  // ...and the clamp is still on the output.
  TEST_ASSERT_EQUAL_INT((int)b.smokePointF - 25, o.fatClampWarnF);
  // And on every subsequent (non-PREP) tick too.
  o = e.step(mk(b.addTempF_lo + 5));
  TEST_ASSERT_EQUAL_INT((int)b.smokePointF - 25, o.fatClampWarnF);
}

static void test_hold_setpoint_carries_through_cue_and_timer(void) {
  // Bench-found (Eggs -> burgers): between HOLD steps the engine used to emit
  // setpointF=0, handing guidance back to whatever stale pre-recipe target was
  // loaded -> instant TOO HOT during "searing side 1". The last HOLD setpoint
  // must ride along on the CUE/TIMER steps that follow it.
  RecipeEngine e;
  e.start(recipe_builtin_smashburger(), 0);
  RecipeOut o = e.step(mk(450));                    // HOLD 450 reached -> CUE
  TEST_ASSERT_EQUAL(int(StepType::CUE), int(o.type));
  TEST_ASSERT_EQUAL_INT(450, o.setpointF);          // carried, not dropped
  TEST_ASSERT_TRUE(o.touchAck);                     // patties: tap OR food-add
  o = e.step(mk(450, /*food*/ true));               // patties in -> TIMER
  TEST_ASSERT_EQUAL(int(StepType::TIMER), int(o.type));
  TEST_ASSERT_EQUAL_INT(450, o.setpointF);
  // TIMER exposes a live countdown (90 s searing step).
  TEST_ASSERT_EQUAL_INT(90, o.secsLeft);
  g_now += 30000;
  o = e.step(mk(450));
  TEST_ASSERT_EQUAL_INT(60, o.secsLeft);
  TEST_ASSERT_FALSE(o.touchAck);                    // passive step: no tap nag
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_smashburger_runs_four_batches);
  RUN_TEST(test_hold_setpoint_carries_through_cue_and_timer);
  RUN_TEST(test_prep_too_hot_for_butter);
  RUN_TEST(test_one_touch_advances_one_gated_step);
  RUN_TEST(test_fat_clamp_survives_prep_advance);
  return UNITY_END();
}
