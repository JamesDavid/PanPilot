// test_recovery — FOOD ADDED detection + recovery through 3 batches, plus the
// slow/fast hints and the disabled case (base spec §6.4, §7.4).
#include <unity.h>
#include "core/recovery.cpp"

namespace {
uint32_t t = 0;
RecoveryMonitor rm;
RecoveryOut feed(float tF, float rate, bool enabled = true) {
  RecoveryOut o = rm.update(tF, rate, /*present*/ true, /*targetLoF*/ 350.0f,
                            enabled, t);
  t += 500;
  return o;
}
}  // namespace

void setUp() { t = 0; rm.reset(); }
void tearDown() {}

static void test_three_batches(void) {
  int recovered = 0;
  for (int b = 0; b < 3; ++b) {
    for (int i = 0; i < 8; ++i) feed(360, 0);          // ~4 s stable & hot
    RecoveryOut o = feed(330, -40);                    // food added: -30 °F
    TEST_ASSERT_EQUAL(int(RecoveryEvent::FOOD_ADDED), int(o.event));
    TEST_ASSERT_TRUE(rm.recovering());
    bool got = false;
    for (float T = 332; T <= 356; T += 6) {            // climb back
      o = feed(T, 40);
      if (o.event == RecoveryEvent::RECOVERED) got = true;
    }
    TEST_ASSERT_TRUE(got);
    TEST_ASSERT_FALSE(rm.recovering());
    ++recovered;
  }
  TEST_ASSERT_EQUAL(3, recovered);
}

static void test_slow_hint(void) {
  for (int i = 0; i < 8; ++i) feed(360, 0);
  feed(330, -40);                                       // food added
  RecoveryOut o = feed(331, 0.5f);                      // barely climbing
  TEST_ASSERT_EQUAL(int(RecoveryHint::SLOW), int(o.hint));
}

static void test_fast_hint(void) {
  for (int i = 0; i < 8; ++i) feed(360, 0);
  feed(330, -40);
  RecoveryOut o = feed(335, 90);                        // recovering very fast
  TEST_ASSERT_EQUAL(int(RecoveryHint::FAST), int(o.hint));
}

static void test_disabled_no_recovery(void) {
  for (int i = 0; i < 8; ++i) feed(360, 0, /*enabled*/ false);
  RecoveryOut o = feed(330, -40, /*enabled*/ false);
  TEST_ASSERT_EQUAL(int(RecoveryEvent::NONE), int(o.event));
  TEST_ASSERT_FALSE(o.recovering);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_three_batches);
  RUN_TEST(test_slow_hint);
  RUN_TEST(test_fast_hint);
  RUN_TEST(test_disabled_no_recovery);
  return UNITY_END();
}
