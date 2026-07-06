// test_interlocks — the full §3.3 S1-S11 truth table. Must be green before any
// control code runs on hardware.
#include <unity.h>
#include "core/control/interlocks.cpp"

namespace {
// A healthy input at time `now` (recent frame/ack/interaction, in-band temp).
InterlockInput healthy(uint32_t now) {
  InterlockInput in;
  in.now = now;
  in.lastFrameMs = now; in.lastAckMs = now; in.lastInteractionMs = now;
  in.confidence = 90; in.presence = PanPresence::PRESENT; in.duty = 0.3f;
  in.rateFPerMin = 5; in.tempF = 300; in.warnF = 450; in.dieTempC = 40;
  return in;
}
}  // namespace

void setUp() {}
void tearDown() {}

static void test_healthy_passes(void) {
  InterlockMonitor m;
  TEST_ASSERT_EQUAL(int(Interlock::NONE), int(m.evaluate(healthy(100000))));
}

static void test_immediate_trips(void) {
  { InterlockMonitor m; auto in = healthy(100000); in.stopPressed = true;
    TEST_ASSERT_EQUAL(int(Interlock::S9_STOP), int(m.evaluate(in))); }
  { InterlockMonitor m; auto in = healthy(100000); in.tempF = 490;   // warn+40
    TEST_ASSERT_EQUAL(int(Interlock::S5_MAXTEMP), int(m.evaluate(in))); }
  { InterlockMonitor m; auto in = healthy(100000); in.tempF = 660;   // absolute
    TEST_ASSERT_EQUAL(int(Interlock::S5_MAXTEMP), int(m.evaluate(in))); }
  { InterlockMonitor m; auto in = healthy(100000); in.sensorFault = true;
    TEST_ASSERT_EQUAL(int(Interlock::S6_SENSOR), int(m.evaluate(in))); }
  { InterlockMonitor m; auto in = healthy(100000); in.lastFrameMs = 90000; // 10 s gap
    TEST_ASSERT_EQUAL(int(Interlock::S6_SENSOR), int(m.evaluate(in))); }
  { InterlockMonitor m; auto in = healthy(100000); in.presence = PanPresence::ABSENT;
    TEST_ASSERT_EQUAL(int(Interlock::S2_PRESENCE), int(m.evaluate(in))); }
  { InterlockMonitor m; auto in = healthy(100000); in.actuatorAlive = false;
    TEST_ASSERT_EQUAL(int(Interlock::S7_ACTUATOR), int(m.evaluate(in))); }
  { InterlockMonitor m; auto in = healthy(100000); in.commsOk = false;
    TEST_ASSERT_EQUAL(int(Interlock::S8_COMMS), int(m.evaluate(in))); }
  { InterlockMonitor m; auto in = healthy(100000); in.dieTempC = 90;
    TEST_ASSERT_EQUAL(int(Interlock::S11_DIEHEAT), int(m.evaluate(in))); }
  { InterlockMonitor m; auto in = healthy(3000000); in.lastInteractionMs = 0;
    TEST_ASSERT_EQUAL(int(Interlock::S10_UNATTENDED), int(m.evaluate(in))); }
}

static void test_dwell_trips(void) {
  // S1 confidence: below floor but only trips after 5 s.
  { InterlockMonitor m; auto a = healthy(1000); a.confidence = 40;
    TEST_ASSERT_EQUAL(int(Interlock::NONE), int(m.evaluate(a)));
    auto b = healthy(7000); b.confidence = 40;
    TEST_ASSERT_EQUAL(int(Interlock::S1_CONFIDENCE), int(m.evaluate(b))); }
  // S3 obstruction: >10 s.
  { InterlockMonitor m; auto a = healthy(1000); a.presence = PanPresence::OBSTRUCTED;
    TEST_ASSERT_EQUAL(int(Interlock::NONE), int(m.evaluate(a)));
    auto b = healthy(12000); b.presence = PanPresence::OBSTRUCTED;
    TEST_ASSERT_EQUAL(int(Interlock::S3_OBSTRUCT), int(m.evaluate(b))); }
  // S4 runaway: high duty, not heating, >60 s.
  { InterlockMonitor m; auto a = healthy(1000); a.duty = 0.8f; a.rateFPerMin = -1;
    TEST_ASSERT_EQUAL(int(Interlock::NONE), int(m.evaluate(a)));
    auto b = healthy(62000); b.duty = 0.8f; b.rateFPerMin = -1;
    TEST_ASSERT_EQUAL(int(Interlock::S4_RUNAWAY), int(m.evaluate(b))); }
}

static void test_emergency_classification(void) {
  TEST_ASSERT_TRUE(InterlockMonitor::isEmergency(Interlock::S5_MAXTEMP));
  TEST_ASSERT_TRUE(InterlockMonitor::isEmergency(Interlock::S4_RUNAWAY));
  TEST_ASSERT_TRUE(InterlockMonitor::isEmergency(Interlock::S7_ACTUATOR));
  TEST_ASSERT_FALSE(InterlockMonitor::isEmergency(Interlock::S2_PRESENCE));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_healthy_passes);
  RUN_TEST(test_immediate_trips);
  RUN_TEST(test_dwell_trips);
  RUN_TEST(test_emergency_classification);
  return UNITY_END();
}
