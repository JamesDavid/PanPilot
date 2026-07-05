// test_power — ACTIVE/IDLE transitions + wake sources (base spec §8).
#include <unity.h>
#include "core/power.cpp"

void setUp() {}
void tearDown() {}

static void test_dims_when_idle_and_cool(void) {
  PowerFsm p;
  TEST_ASSERT_EQUAL(int(PowerState::ACTIVE), int(p.state()));
  p.step({IDLE_TIMEOUT_MS + 1, /*hot*/ false, /*heat*/ false});
  TEST_ASSERT_EQUAL(int(PowerState::IDLE), int(p.state()));
}

static void test_stays_active_while_hot(void) {
  PowerFsm p;
  p.step({IDLE_TIMEOUT_MS + 1, /*hot*/ true, false});
  TEST_ASSERT_EQUAL(int(PowerState::ACTIVE), int(p.state()));  // cooking, no dim
}

static void test_touch_wakes(void) {
  PowerFsm p;
  p.step({IDLE_TIMEOUT_MS + 1, false, false});          // -> IDLE
  p.step({0, false, false});                            // recent touch
  TEST_ASSERT_EQUAL(int(PowerState::ACTIVE), int(p.state()));
  TEST_ASSERT_TRUE(p.wokeThisStep());
  TEST_ASSERT_FALSE(p.wokeByHeat());
}

static void test_heat_wakes(void) {
  PowerFsm p;
  p.step({IDLE_TIMEOUT_MS + 1, false, false});          // -> IDLE
  p.step({IDLE_TIMEOUT_MS + 5000, false, /*heat*/ true});
  TEST_ASSERT_EQUAL(int(PowerState::ACTIVE), int(p.state()));
  TEST_ASSERT_TRUE(p.wokeThisStep());
  TEST_ASSERT_TRUE(p.wokeByHeat());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_dims_when_idle_and_cool);
  RUN_TEST(test_stays_active_while_hot);
  RUN_TEST(test_touch_wakes);
  RUN_TEST(test_heat_wakes);
  return UNITY_END();
}
