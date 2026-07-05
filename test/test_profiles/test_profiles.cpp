// test_profiles — Learn Pan lag heuristic + profile construction (base spec §7).
#include <unity.h>
#include "core/profiles.cpp"

void setUp() {}
void tearDown() {}

static void test_lag_monotonic_and_clamped(void) {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.3f, learn_lag_from_rate(0));
  TEST_ASSERT_TRUE(learn_lag_from_rate(100) > learn_lag_from_rate(40));  // faster => more lag
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, learn_lag_from_rate(1000));     // clamped
  TEST_ASSERT_TRUE(learn_lag_from_rate(50) >= 0.3f);
}

static void test_make_profile(void) {
  PanProfile p = make_profile("Cast Iron", 80.0f);
  TEST_ASSERT_TRUE(p.valid);
  TEST_ASSERT_EQUAL_UINT16(PANPROFILE_MAGIC, p.magic);
  TEST_ASSERT_EQUAL_FLOAT(learn_lag_from_rate(80.0f), p.lagMinutes);
  TEST_ASSERT_EQUAL_STRING("Cast Iron", p.name);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_lag_monotonic_and_clamped);
  RUN_TEST(test_make_profile);
  return UNITY_END();
}
