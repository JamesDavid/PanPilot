// test_foodtimer — temperature-compensated doneness accumulator, FLIP/REMOVE
// cues, and the cold-pan stretch (roadmap §2.7).
#include <unity.h>
#include "core/foodtimer.cpp"
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
  RUN_TEST(test_poultry_has_safety_temp);
  return UNITY_END();
}
