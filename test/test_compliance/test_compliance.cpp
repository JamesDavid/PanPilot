// test_compliance — knob-turn compliance verification (roadmap §3.5).
#include <unity.h>
#include "core/compliance.cpp"

void setUp() {}
void tearDown() {}

static void test_turn_down_complied(void) {
  ComplianceChecker c;
  c.start(/*expectFall*/ true, 0);
  TEST_ASSERT_EQUAL(int(Compliance::PENDING), int(c.update(40, 100)));
  TEST_ASSERT_EQUAL(int(Compliance::COMPLIED), int(c.update(5, 3000)));  // fell
}

static void test_turn_down_failed(void) {
  ComplianceChecker c;
  c.start(true, 0);
  TEST_ASSERT_EQUAL(int(Compliance::PENDING), int(c.update(40, 5000)));
  TEST_ASSERT_EQUAL(int(Compliance::FAILED), int(c.update(40, 21000)));  // timed out
}

static void test_turn_up_complied(void) {
  ComplianceChecker c;
  c.start(/*expectFall*/ false, 0);
  TEST_ASSERT_EQUAL(int(Compliance::COMPLIED), int(c.update(30, 2000)));  // rose
}

static void test_sticky_after_result(void) {
  ComplianceChecker c;
  c.start(true, 0);
  c.update(5, 1000);                                   // COMPLIED
  TEST_ASSERT_EQUAL(int(Compliance::COMPLIED), int(c.update(40, 9000)));  // stays
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_turn_down_complied);
  RUN_TEST(test_turn_down_failed);
  RUN_TEST(test_turn_up_complied);
  RUN_TEST(test_sticky_after_result);
  return UNITY_END();
}
