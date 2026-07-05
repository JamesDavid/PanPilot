// test_thermal_model — smoothing convergence, least-squares rate against
// synthetic ramps, trend classification, absent-ignored (base spec §7.1, §11).
#include <unity.h>

#include "core/thermal_model.cpp"  // include-the-TU

namespace {
PanReading rd(float c, uint32_t t_ms, PanPresence p = PanPresence::PRESENT) {
  PanReading r{};
  r.panTempC = c; r.t_ms = t_ms; r.presence = p; r.confidence = 80;
  return r;
}
}  // namespace

void setUp() {}
void tearDown() {}

// Smoothing does not jump instantly to a step input; it eases toward it.
static void test_smoothing_eases(void) {
  ThermalModel m;
  m.update(rd(0.0f, 0));                 // disp initialized to 0
  m.update(rd(100.0f, 250));             // one step toward 100
  TEST_ASSERT_TRUE(m.displayTempC() > 0.0f);
  TEST_ASSERT_TRUE(m.displayTempC() < 100.0f);   // eased, not jumped
  for (uint32_t t = 500; t <= 12000; t += 250) m.update(rd(100.0f, t));
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 100.0f, m.displayTempC());   // converged
}

// A linear ramp of +60 °C/min is recovered by the slope fit.
static void test_rate_matches_ramp(void) {
  ThermalModel m;
  const float ratePerMin = 60.0f;
  for (uint32_t t = 0; t <= 12000; t += 250)
    m.update(rd(20.0f + ratePerMin * (t / 60000.0f), t));
  TEST_ASSERT_FLOAT_WITHIN(4.0f, 60.0f, m.rateCPerMin());
  TEST_ASSERT_EQUAL(static_cast<int>(Trend::RISING_FAST),
                    static_cast<int>(m.trend()));
}

// Rate is withheld ("estimating") until >=5 s of span exists.
static void test_rate_estimating_below_5s(void) {
  ThermalModel m;
  for (uint32_t t = 0; t <= 3000; t += 250)
    m.update(rd(20.0f + 60.0f * (t / 60000.0f), t));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, m.rateCPerMin());
}

static void test_stable_and_falling(void) {
  ThermalModel m;
  for (uint32_t t = 0; t <= 12000; t += 250) m.update(rd(150.0f, t));  // flat
  TEST_ASSERT_EQUAL(static_cast<int>(Trend::STABLE),
                    static_cast<int>(m.trend()));
  ThermalModel d;
  for (uint32_t t = 0; t <= 12000; t += 250)
    d.update(rd(300.0f - 40.0f * (t / 60000.0f), t));  // -40 C/min
  TEST_ASSERT_EQUAL(static_cast<int>(Trend::COOLING),
                    static_cast<int>(d.trend()));
}

static void test_absent_ignored(void) {
  ThermalModel m;
  m.update(rd(200.0f, 0, PanPresence::ABSENT));
  TEST_ASSERT_FALSE(m.valid());          // absent reading did not advance model
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_smoothing_eases);
  RUN_TEST(test_rate_matches_ramp);
  RUN_TEST(test_rate_estimating_below_5s);
  RUN_TEST(test_stable_and_falling);
  RUN_TEST(test_absent_ignored);
  return UNITY_END();
}
