// test_session — session accumulation: max temp, time-in-range, overheat,
// time-to-target, min confidence (base spec §8).
#include <unity.h>
#include "core/session.cpp"

namespace {
PanReading rd(float c, uint8_t conf = 90, PanPresence p = PanPresence::PRESENT) {
  PanReading r{}; r.panTempC = c; r.confidence = conf; r.presence = p; return r;
}
}  // namespace

void setUp() {}
void tearDown() {}

static void test_accumulates(void) {
  SessionAccumulator s;
  s.begin(2 /*preset*/, 0);
  uint32_t t = 0;
  // heat up (HEAT_MORE) for 10 s
  for (; t <= 10000; t += 1000)
    s.update(rd(100 + t / 200.0f, 80), GuidanceState::HEAT_MORE, t);
  // in range (READY) for 20 s
  for (; t <= 30000; t += 1000) s.update(rd(175, 70), GuidanceState::READY, t);
  // overheat for 5 s
  for (; t <= 35000; t += 1000) s.update(rd(260, 60), GuidanceState::TOO_HOT, t);

  SessionSummary sum = s.finish(t);
  TEST_ASSERT_EQUAL_UINT8(2, sum.presetId);
  TEST_ASSERT_TRUE(sum.maxTempC >= 260.0f);
  TEST_ASSERT_INT_WITHIN(2, 20, (int)sum.timeInRangeSec);   // ~20 s in range
  TEST_ASSERT_INT_WITHIN(2, 5, (int)sum.overheatSec);       // ~5 s overheat
  TEST_ASSERT_TRUE(sum.timeToTargetSec >= 10 && sum.timeToTargetSec <= 12);
  TEST_ASSERT_TRUE(sum.minConfidence <= 60);
}

static void test_target_never_reached(void) {
  SessionAccumulator s;
  s.begin(5, 0);
  for (uint32_t t = 0; t <= 10000; t += 1000)
    s.update(rd(80), GuidanceState::HEAT_MORE, t);
  SessionSummary sum = s.finish(10000);
  TEST_ASSERT_EQUAL_INT32(-1, sum.timeToTargetSec);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_accumulates);
  RUN_TEST(test_target_never_reached);
  return UNITY_END();
}
