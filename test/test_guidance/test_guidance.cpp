// test_guidance — scripted scenarios through the guidance state machine:
// preheat happy path (HEAT_MORE/HOLD -> READY + chime), overshoot prediction
// (TURN_DOWN_NOW before target), TURN_DOWN_SOON, TOO_HOT alarm cadence, NO_PAN,
// READY re-arm (base spec §7.2, §11).
#include <unity.h>
#include "core/guidance.cpp"  // include-the-TU

namespace {
uint32_t g_now = 0;

GuidanceInput mk(float tempF, float rate, uint8_t conf = 80,
                 PanPresence p = PanPresence::PRESENT, bool moved = false) {
  GuidanceInput in;
  in.tempF = tempF; in.rateFPerMin = rate; in.confidence = conf;
  in.presence = p; in.moved = moved;
  return in;
}

// Step the engine `ticks` times (250 ms apart) at a fixed condition; return last.
GuidanceOutput run(GuidanceEngine& g, const Target& t, const GuidanceInput& in,
                   int ticks) {
  GuidanceOutput o;
  for (int i = 0; i < ticks; ++i) { o = g.step(in, t, g_now); g_now += 250; }
  return o;
}
}  // namespace

void setUp() { g_now = 0; }
void tearDown() {}

static Target tgt350() { Target t; t.setCenter(350); return t; }  // lo340 hi360 warn450

static void test_preheat_reaches_ready_and_chimes(void) {
  GuidanceEngine g; Target t = tgt350();
  run(g, t, mk(150, 15), 4);                       // well below, rising slow
  TEST_ASSERT_EQUAL(int(GuidanceState::HOLD), int(g.state()));
  // climb to the band
  bool chimed = false;
  for (float T = 200; T <= 350; T += 10) {
    GuidanceOutput o = run(g, t, mk(T, 15), 2);
    if (o.alert == AlertAction::READY_CHIME) chimed = true;
  }
  TEST_ASSERT_EQUAL(int(GuidanceState::READY), int(g.state()));
  TEST_ASSERT_TRUE(chimed);
}

static void test_overshoot_turn_down_before_target(void) {
  GuidanceEngine g; Target t = tgt350();
  // 300 °F but +200 °F/min -> projected peak 400 > hi(360): TURN_DOWN_NOW early.
  GuidanceOutput o = run(g, t, mk(300, 200), 3);
  TEST_ASSERT_EQUAL(int(GuidanceState::TURN_DOWN_NOW), int(o.state));
  TEST_ASSERT_TRUE(o.projectedPeakF > 360.0f);
}

static void test_turn_down_soon(void) {
  GuidanceEngine g; Target t = tgt350();
  // 332 °F (within 15 of lo 340), rising 30 °F/min (medium), projPeak 347 <= 360.
  GuidanceOutput o = run(g, t, mk(332, 30), 3);
  TEST_ASSERT_EQUAL(int(GuidanceState::TURN_DOWN_SOON), int(o.state));
}

static void test_too_hot_alarm_cadence(void) {
  GuidanceEngine g; Target t = tgt350();
  GuidanceOutput o = run(g, t, mk(460, 0), 3);     // above warn 450
  TEST_ASSERT_EQUAL(int(GuidanceState::TOO_HOT), int(o.state));
  // alarm fired at least once during entry
  // advance <10 s: no repeat; >10 s: repeats
  g_now += 5000; GuidanceOutput a = g.step(mk(460, 0), t, g_now);
  TEST_ASSERT_EQUAL(int(AlertAction::NONE), int(a.alert));
  g_now += 6000; GuidanceOutput b = g.step(mk(460, 0), t, g_now);
  TEST_ASSERT_EQUAL(int(AlertAction::TOO_HOT_ALARM), int(b.alert));
}

static void test_stainless_suppresses_toohot_below_300(void) {
  Target t; t.setCenter(180);        // lo170 hi190 warn280 (a low target)
  GuidanceInput hot = mk(282, 0);    // >= warn 280 but < 300 °F
  // Non-stainless: a genuine TOO_HOT.
  { GuidanceEngine g; TEST_ASSERT_EQUAL(int(GuidanceState::TOO_HOT),
                                        int(run(g, t, hot, 3).state)); }
  // Stainless: suppressed to LOW_CONFIDENCE (trust the trend).
  GuidanceInput hotS = hot; hotS.stainlessHint = true;
  { GuidanceEngine g; TEST_ASSERT_EQUAL(int(GuidanceState::LOW_CONFIDENCE),
                                        int(run(g, t, hotS, 3).state)); }
  // Stainless but >= 300 °F: NOT suppressed — that's a real overheat.
  GuidanceInput reallyHot = mk(320, 0); reallyHot.stainlessHint = true;
  { GuidanceEngine g; TEST_ASSERT_EQUAL(int(GuidanceState::TOO_HOT),
                                        int(run(g, t, reallyHot, 3).state)); }
}

static void test_alternating_classification_cannot_starve(void) {
  // Stainless spread hovering at the threshold makes classify() alternate
  // TOO_HOT / LOW_CONFIDENCE every tick. Each flip used to reset the 2-tick
  // confirm counter, freezing state_ at the stale pre-overheat value forever —
  // no alarm while the pan sat above warn. The starvation guard must adopt
  // one of the live classifications within 2x the hysteresis window.
  GuidanceEngine g;
  Target t; t.setCenter(180);          // warn 280, so 282 °F is over warn
  run(g, t, mk(180, 0), 4);            // settle in READY (the stale state)
  TEST_ASSERT_EQUAL(int(GuidanceState::READY), int(g.state()));
  bool leftReady = false;
  for (int i = 0; i < 8; ++i) {        // alternate the two overheat readings
    GuidanceInput in = mk(282, 0);
    in.stainlessHint = (i & 1);        // flickering stainless signature
    g.step(in, t, g_now); g_now += 250;
    if (g.state() != GuidanceState::READY) { leftReady = true; break; }
  }
  TEST_ASSERT_TRUE(leftReady);
  TEST_ASSERT_TRUE(g.state() == GuidanceState::TOO_HOT ||
                   g.state() == GuidanceState::LOW_CONFIDENCE);
}

static void test_no_pan(void) {
  GuidanceEngine g; Target t = tgt350();
  GuidanceOutput o = run(g, t, mk(0, 0, 0, PanPresence::ABSENT), 3);
  TEST_ASSERT_EQUAL(int(GuidanceState::NO_PAN), int(o.state));
}

static void test_ready_rearms_after_leaving(void) {
  GuidanceEngine g; Target t = tgt350();
  int chimes = 0;
  auto feed = [&](float T, int n) {
    for (int i = 0; i < n; ++i) {
      GuidanceOutput o = g.step(mk(T, 0), t, g_now); g_now += 250;
      if (o.alert == AlertAction::READY_CHIME) ++chimes;
    }
  };
  feed(350, 4);        // READY -> chime #1
  feed(350, 4);        // still ready -> no repeat
  feed(300, 4);        // leaves band by >10 -> re-arm
  feed(350, 4);        // READY again -> chime #2
  TEST_ASSERT_EQUAL(2, chimes);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_preheat_reaches_ready_and_chimes);
  RUN_TEST(test_overshoot_turn_down_before_target);
  RUN_TEST(test_turn_down_soon);
  RUN_TEST(test_too_hot_alarm_cadence);
  RUN_TEST(test_stainless_suppresses_toohot_below_300);
  RUN_TEST(test_alternating_classification_cannot_starve);
  RUN_TEST(test_no_pan);
  RUN_TEST(test_ready_rearms_after_leaving);
  UNITY_END();
  return 0;
}
