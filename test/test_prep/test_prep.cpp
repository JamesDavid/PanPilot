// test_prep — the runtime fat monitor (roadmap §4.1.1): add-window detection,
// melt/equalize/immediate ready criteria, and the smoke-point clamp.
#include <unity.h>
#include "core/preplib.h"
#include <cstring>

namespace {
int prep_id(const char* name) {
  for (int i = 0; i < preplib_count(); ++i)
    if (std::strcmp(preplib_entry(i).name, name) == 0) return i;
  return 0;
}
}  // namespace

void setUp() {}
void tearDown() {}

static void test_butter_too_hot(void) {
  const int id = prep_id("Butter");
  const PrepEntry& p = preplib_entry(id);
  PrepStatus s = prep_evaluate(id, p.maxAddTempF + 50, 0, 0, false);
  TEST_ASSERT_TRUE(s.tooHot);
  TEST_ASSERT_EQUAL_INT(p.addTempF_hi, s.coolToF);
  TEST_ASSERT_EQUAL_INT(p.smokePointF - 25, s.fatClampWarnF);   // fat-state clamp
}

static void test_butter_below_window(void) {
  const int id = prep_id("Butter");
  const PrepEntry& p = preplib_entry(id);
  PrepStatus s = prep_evaluate(id, p.addTempF_lo - 30, 0, 0, false);
  TEST_ASSERT_TRUE(s.belowWindow);
  TEST_ASSERT_FALSE(s.ready);
}

static void test_butter_needs_melt_dwell(void) {
  const int id = prep_id("Butter");
  const PrepEntry& p = preplib_entry(id);
  const float mid = (p.addTempF_lo + p.addTempF_hi) / 2.0f;
  TEST_ASSERT_FALSE(prep_evaluate(id, mid, 0, 5000, false).ready);      // 5 s
  TEST_ASSERT_TRUE(prep_evaluate(id, mid, 0, PREP_MELT_DWELL_MS, false).ready);
  TEST_ASSERT_TRUE(prep_evaluate(id, mid, 0, 0, false).advice[0] != '\0');
}

static void test_oil_needs_equalize(void) {
  const int id = prep_id("Olive oil (EVOO)");
  const PrepEntry& p = preplib_entry(id);
  const float mid = (p.addTempF_lo + p.addTempF_hi) / 2.0f;
  TEST_ASSERT_FALSE(prep_evaluate(id, mid, 40.0f, 0, false).ready);   // still climbing
  TEST_ASSERT_TRUE(prep_evaluate(id, mid, 2.0f, 0, false).ready);     // equalized
}

static void test_water_immediate_no_clamp(void) {
  const int id = prep_id("Water (steam/pot-stickers)");
  const PrepEntry& p = preplib_entry(id);
  PrepStatus s = prep_evaluate(id, (p.addTempF_lo + p.addTempF_hi) / 2.0f, 30, 0, false);
  TEST_ASSERT_TRUE(s.ready);
  TEST_ASSERT_EQUAL_INT(0, s.fatClampWarnF);   // water has no smoke clamp
}

static void test_brown_on_purpose_relaxes_clamp(void) {
  const int id = prep_id("Butter");
  const PrepEntry& p = preplib_entry(id);
  PrepStatus s = prep_evaluate(id, p.addTempF_lo, 0, 0, /*brownOnPurpose*/ true);
  TEST_ASSERT_EQUAL_INT(p.smokePointF, s.fatClampWarnF);   // full smoke point, not -25
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_butter_too_hot);
  RUN_TEST(test_butter_below_window);
  RUN_TEST(test_butter_needs_melt_dwell);
  RUN_TEST(test_oil_needs_equalize);
  RUN_TEST(test_water_immediate_no_clamp);
  RUN_TEST(test_brown_on_purpose_relaxes_clamp);
  return UNITY_END();
}
