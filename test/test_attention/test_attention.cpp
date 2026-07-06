// test_attention — escalation levels, beep cadence, strobe, mute (roadmap §3.5).
#include <unity.h>
#include "core/attention.cpp"

void setUp() {}
void tearDown() {}

static void test_l1_beeps_once(void) {
  AttentionManager a;
  a.raise(AttnLevel::L1, "READY", "", 0);
  TEST_ASSERT_TRUE(a.tick(0).buzz);          // entry beep
  TEST_ASSERT_FALSE(a.tick(250).buzz);       // L1 doesn't repeat
  TEST_ASSERT_FALSE(a.tick(9000).buzz);
}

static void test_l2_repeats_and_strobes(void) {
  AttentionManager a;
  a.raise(AttnLevel::L2, "TURN DOWN NOW", "", 1000);
  AttnOutput o = a.tick(1000);
  TEST_ASSERT_TRUE(o.buzz);
  TEST_ASSERT_TRUE(o.strobe);
  TEST_ASSERT_FALSE(a.tick(3000).buzz);      // <5 s since last
  TEST_ASSERT_TRUE(a.tick(6001).buzz);       // >=5 s -> repeat
}

static void test_l3_fast_repeat(void) {
  AttentionManager a;
  a.raise(AttnLevel::L3, "TOO HOT", "", 0);
  TEST_ASSERT_TRUE(a.tick(0).buzz);
  TEST_ASSERT_FALSE(a.tick(1000).buzz);
  TEST_ASSERT_TRUE(a.tick(2001).buzz);       // >=2 s
}

static void test_mute_suppresses_below_l3(void) {
  AttentionManager a;
  a.setMuted(true);
  a.raise(AttnLevel::L2, "x", "", 0);
  TEST_ASSERT_FALSE(a.tick(0).buzz);         // muted L2 silent
  TEST_ASSERT_TRUE(a.tick(0).strobe);        // ...but still strobes
  a.raise(AttnLevel::L3, "y", "", 0);
  TEST_ASSERT_TRUE(a.tick(0).buzz);          // L3 never muted
}

static void test_same_cue_no_retrigger(void) {
  AttentionManager a;
  a.raise(AttnLevel::L2, "z", "", 0);
  TEST_ASSERT_TRUE(a.tick(0).buzz);
  a.raise(AttnLevel::L2, "z", "", 100);      // identical -> no reset
  TEST_ASSERT_FALSE(a.tick(200).buzz);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_l1_beeps_once);
  RUN_TEST(test_l2_repeats_and_strobes);
  RUN_TEST(test_l3_fast_repeat);
  RUN_TEST(test_mute_suppresses_below_l3);
  RUN_TEST(test_same_cue_no_retrigger);
  return UNITY_END();
}
