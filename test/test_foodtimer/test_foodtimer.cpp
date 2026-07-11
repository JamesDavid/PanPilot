// test_foodtimer — temperature-compensated doneness accumulator, FLIP/REMOVE
// cues, and the cold-pan stretch (roadmap §2.7).
#include <unity.h>
#include "core/foodtimer.cpp"
#include "core/foodlib.cpp"   // foodlib_count/entry now live here (custom merge)
#include <cstring>

namespace {
const FoodEntry* find(const char* name) {
  for (int i = 0; i < foodlib_count(); ++i)
    if (strcmp(foodlib_entry(i).name, name) == 0) return &foodlib_entry(i);
  return nullptr;
}

// Run at a fixed pan temp in 5 s steps until `ev`; return elapsed wall seconds.
int time_to(FoodTimer& ft, float panF, FoodTimerOut::Event ev, int limit = 2000) {
  uint32_t t = 0;
  for (int s = 0; s < limit; s += 5) {
    t += 5000;
    FoodTimerOut o = ft.update(panF, t);
    if (o.event == ev) return s + 5;
  }
  return -1;
}
}  // namespace

void setUp() {}
void tearDown() {}

static void test_flip_at_reference_time(void) {
  const FoodEntry* p = find("Pancakes");      // sideSec[0]=150 @ ref 365
  TEST_ASSERT_NOT_NULL(p);
  FoodTimer ft; ft.start(p, 0);
  int tf = time_to(ft, p->refTempF, FoodTimerOut::FLIP);
  TEST_ASSERT_INT_WITHIN(8, 150, tf);         // ~150 s at reference temp
}

static void test_remove_after_second_side(void) {
  const FoodEntry* p = find("Pancakes");
  FoodTimer ft; ft.start(p, 0);
  time_to(ft, p->refTempF, FoodTimerOut::FLIP);
  int tr = time_to(ft, p->refTempF, FoodTimerOut::REMOVE);
  TEST_ASSERT_INT_WITHIN(8, 90, tr);          // side 2 ~90 s
}

static void test_cold_pan_stretches(void) {
  const FoodEntry* p = find("Pancakes");
  FoodTimer ft; ft.start(p, 0);
  int tf = time_to(ft, p->refTempF - 40, FoodTimerOut::FLIP);
  TEST_ASSERT_TRUE(tf > 165);                 // colder -> longer than 150 s
}

static void test_comp_factor(void) {
  const FoodEntry* p = find("Pancakes");
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, FoodTimer::compFactor(p, p->refTempF));
  TEST_ASSERT_TRUE(FoodTimer::compFactor(p, p->refTempF + 25) > 1.0f);   // hotter faster
  TEST_ASSERT_TRUE(FoodTimer::compFactor(p, p->refTempF - 25) < 1.0f);   // colder slower
  TEST_ASSERT_TRUE(FoodTimer::compFactor(p, 0) >= 0.7f);                 // clamped
}

static void test_time_factor_scales_and_pct_tracks(void) {
  // Personalization (spec §2.7): timeFactor 1.5 stretches side 1 from 150 s to
  // ~225 s, and progressPct must track the SCALED target — the old UI computed
  // percent from the seed time, which reads negative early in a stretched cook.
  const FoodEntry* p = find("Pancakes");
  FoodTimer ft; ft.start(p, 0, /*timeFactor=*/1.5f);
  uint32_t t = 0;
  uint8_t firstPct = 255, prevPct = 0;
  int flipAt = -1;
  for (int s = 0; s < 400; s += 5) {
    t += 5000;
    FoodTimerOut o = ft.update(p->refTempF, t);
    if (firstPct == 255) firstPct = o.progressPct;
    TEST_ASSERT_TRUE(o.progressPct >= prevPct || o.event == FoodTimerOut::FLIP);
    prevPct = (o.event == FoodTimerOut::FLIP) ? 0 : o.progressPct;
    if (o.event == FoodTimerOut::FLIP) { flipAt = s + 5; break; }
  }
  TEST_ASSERT_INT_WITHIN(8, 225, flipAt);       // 150 * 1.5
  TEST_ASSERT_TRUE(firstPct <= 5);              // starts near 0, not clamped-wrong
}

static void test_flip_tick_reports_new_side(void) {
  // On the FLIP tick, side/remaining/percent must describe the NEW side (the
  // old code returned the old side's ~0 remaining with the new side number).
  const FoodEntry* p = find("Pancakes");        // 150 s / 90 s
  FoodTimer ft; ft.start(p, 0);
  uint32_t t = 0;
  for (int s = 0; s < 400; s += 5) {
    t += 5000;
    FoodTimerOut o = ft.update(p->refTempF, t);
    if (o.event == FoodTimerOut::FLIP) {
      TEST_ASSERT_EQUAL_UINT8(2, o.side);
      TEST_ASSERT_INT_WITHIN(10, 90, o.remainingSec);   // side 2's full time
      TEST_ASSERT_TRUE(o.progressPct <= 5);
      return;
    }
  }
  TEST_FAIL_MESSAGE("never flipped");
}

static void test_poultry_has_safety_temp(void) {
  const FoodEntry* c = find("Chicken breast");
  TEST_ASSERT_NOT_NULL(c);
  TEST_ASSERT_EQUAL_UINT16(165, c->safeInternalF);   // never zeroed
  TEST_ASSERT_EQUAL_UINT16(0, find("Pancakes")->safeInternalF);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_flip_at_reference_time);
  RUN_TEST(test_remove_after_second_side);
  RUN_TEST(test_cold_pan_stretches);
  RUN_TEST(test_comp_factor);
  RUN_TEST(test_time_factor_scales_and_pct_tracks);
  RUN_TEST(test_flip_tick_reports_new_side);
  RUN_TEST(test_poultry_has_safety_temp);
  return UNITY_END();
}
