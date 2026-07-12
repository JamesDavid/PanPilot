// test_burnermap — the per-pan burner calibration: run the full wizard
// protocol against a first-order plant with 5 knob power levels, then check
// the solved settle temperatures and knob suggestions against ground truth.
#include <unity.h>
#include "core/burnermap.cpp"
#include <cmath>

namespace {
// Plant in °F: dT/dt = P(knob) - kLoss*(T - amb).  Truth: Tss = amb + P/kLoss.
struct Plant {
  float t = 75, amb = 75;
  float kLossPerSec = 0.004f;                       // 0.24 per minute
  float pPerSec[BURNER_SETTINGS] = {0.30f, 0.55f, 0.90f, 1.40f, 2.20f};
  int knob = 0;      // -1 = off
  void step(float dt) {
    const float p = knob >= 0 ? pPerSec[knob] : 0.0f;
    t += (p - kLossPerSec * (t - amb)) * dt;
  }
  float settleF(int s) const { return amb + pPerSec[s] / kLossPerSec; }
};

// Drive the mapper through the whole wizard protocol at 1 s ticks.
BurnerMap run_wizard(Plant& p) {
  BurnerMapper m;
  uint32_t now = 0;
  m.start(p.amb, now);
  for (int guard = 0; guard < 4000 && m.phase() != BurnerMapper::DONE; ++guard) {
    if (m.phase() == BurnerMapper::AWAIT_KNOB) { p.knob = m.setting(); m.confirm(now); }
    else if (m.phase() == BurnerMapper::AWAIT_OFF) { p.knob = -1; m.confirm(now); }
    p.step(1.0f);
    now += 1000;
    m.update(p.t, now);
  }
  return m.result();
}
}  // namespace

void setUp() {}
void tearDown() {}

static void test_wizard_completes_and_is_valid(void) {
  Plant p;
  BurnerMap map = run_wizard(p);
  TEST_ASSERT_TRUE(map.valid);
  TEST_ASSERT_TRUE(burnermap_kloss(map) > 0.1f);   // ~0.24/min ground truth
  TEST_ASSERT_TRUE(burnermap_kloss(map) < 0.5f);
}

static void test_settle_predictions_near_truth(void) {
  Plant p;
  BurnerMap map = run_wizard(p);
  for (int s = 0; s < BURNER_SETTINGS; ++s) {
    const float truth = p.settleF(s);              // 150..625 F for these P's
    const float pred = burnermap_settleF(map, s);
    // The wizard measures short windows on a moving plant — allow 15% error.
    TEST_ASSERT_FLOAT_WITHIN(truth * 0.15f, truth, pred);
  }
}

static void test_suggest_picks_right_knob(void) {
  Plant p;
  BurnerMap map = run_wizard(p);
  // Ask for each setting's own true settle temp: the suggestion must match.
  for (int s = 0; s < BURNER_SETTINGS; ++s)
    TEST_ASSERT_EQUAL_INT(s, burnermap_suggest(map, p.settleF(s)));
  TEST_ASSERT_EQUAL_STRING("MED", burnermap_suggest_name(map, p.settleF(2)));
}

static void test_invalid_map_suggests_nothing(void) {
  BurnerMap m{};                                    // never measured
  TEST_ASSERT_EQUAL_INT(-1, burnermap_suggest(m, 350));
  TEST_ASSERT_NULL(burnermap_suggest_name(m, 350));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_wizard_completes_and_is_valid);
  RUN_TEST(test_settle_predictions_near_truth);
  RUN_TEST(test_suggest_picks_right_knob);
  RUN_TEST(test_invalid_map_suggests_nothing);
  return UNITY_END();
}
