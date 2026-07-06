// test_settings — brightness level mappings: monotonic PWM, wrap-around cycle,
// clamping, and names (Phase 2 Settings screen).
#include <unity.h>
#include "core/settings.h"

using namespace panpilot;

void setUp() {}
void tearDown() {}

static void test_pwm_monotonic(void) {
  TEST_ASSERT_TRUE(brightness_pwm(0) < brightness_pwm(1));
  TEST_ASSERT_TRUE(brightness_pwm(1) < brightness_pwm(2));
  TEST_ASSERT_EQUAL_UINT8(255, brightness_pwm(2));
}

static void test_cycle_wraps(void) {
  TEST_ASSERT_EQUAL_UINT8(1, brightness_cycle(0));
  TEST_ASSERT_EQUAL_UINT8(2, brightness_cycle(1));
  TEST_ASSERT_EQUAL_UINT8(0, brightness_cycle(2));   // High -> Low
}

static void test_clamp(void) {
  TEST_ASSERT_EQUAL_UINT8(0, brightness_clamp(-5));
  TEST_ASSERT_EQUAL_UINT8(2, brightness_clamp(9));
  TEST_ASSERT_EQUAL_UINT8(1, brightness_clamp(1));
}

static void test_names(void) {
  TEST_ASSERT_EQUAL_STRING("Low", brightness_name(0));
  TEST_ASSERT_EQUAL_STRING("Medium", brightness_name(1));
  TEST_ASSERT_EQUAL_STRING("High", brightness_name(2));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_pwm_monotonic);
  RUN_TEST(test_cycle_wraps);
  RUN_TEST(test_clamp);
  RUN_TEST(test_names);
  return UNITY_END();
}
