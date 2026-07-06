// test_timezones — the timezone table is well-formed and the cycle/clamp
// helpers behave (NTP clock setting).
#include <unity.h>
#include "core/timezones.h"
#include <cstring>

void setUp() {}
void tearDown() {}

static void test_table_wellformed(void) {
  TEST_ASSERT_TRUE(tz_count() >= 8);
  for (int i = 0; i < tz_count(); ++i) {
    TEST_ASSERT_NOT_NULL(tz_name(i));
    TEST_ASSERT_NOT_NULL(tz_posix(i));
    TEST_ASSERT_TRUE(std::strlen(tz_name(i)) > 0);
    TEST_ASSERT_TRUE(std::strlen(tz_posix(i)) > 0);
  }
}

static void test_clamp(void) {
  TEST_ASSERT_EQUAL_INT(0, tz_clamp(-3));
  TEST_ASSERT_EQUAL_INT(tz_count() - 1, tz_clamp(999));
  TEST_ASSERT_EQUAL_INT(2, tz_clamp(2));
}

static void test_cycle_wraps(void) {
  TEST_ASSERT_EQUAL_INT(1, tz_cycle(0));
  TEST_ASSERT_EQUAL_INT(0, tz_cycle(tz_count() - 1));   // wraps to first
}

static void test_known_zones_present(void) {
  bool utc = false, eastern = false;
  for (int i = 0; i < tz_count(); ++i) {
    if (!std::strcmp(tz_name(i), "UTC")) utc = true;
    if (!std::strcmp(tz_name(i), "US Eastern")) eastern = true;
  }
  TEST_ASSERT_TRUE(utc);
  TEST_ASSERT_TRUE(eastern);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_table_wellformed);
  RUN_TEST(test_clamp);
  RUN_TEST(test_cycle_wraps);
  RUN_TEST(test_known_zones_present);
  return UNITY_END();
}
