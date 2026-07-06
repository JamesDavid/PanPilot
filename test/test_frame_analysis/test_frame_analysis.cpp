// test_frame_analysis — synthetic 32x24 frames exercise blob selection,
// interior-percentile pan temperature, presence, and the stainless signature
// (base spec §6, §11). The module is pure C++; compile it straight in.
#include <unity.h>
#include <cmath>

#include "sensor/frame_analysis.cpp"  // include-the-TU: no test_build_src needed

namespace {
constexpr float AMB = 25.0f;

ThermalFrame make_uniform(float t, uint32_t t_ms) {
  ThermalFrame f{};
  f.ambientC = AMB; f.t_ms = t_ms; f.valid = true;
  for (int r = 0; r < THERM_ROWS; ++r)
    for (int c = 0; c < THERM_COLS; ++c) f.px[r][c] = t;
  return f;
}

// Filled hot disc of radius `rad` at (cx,cy) over an ambient field.
ThermalFrame make_disc(float cx, float cy, float rad, float hot,
                       uint32_t t_ms = 0, float amb = AMB) {
  ThermalFrame f = make_uniform(amb, t_ms);
  for (int r = 0; r < THERM_ROWS; ++r)
    for (int c = 0; c < THERM_COLS; ++c) {
      const float d = std::hypot(c - cx, r - cy);
      if (d <= rad) f.px[r][c] = hot;
    }
  return f;
}
}  // namespace

void setUp() {}
void tearDown() {}

static void test_centered_disc_present(void) {
  FrameAnalyzer fa;
  auto f = make_disc(16, 12, 5, 200.0f, 100);
  PanReading r = fa.process(f);
  TEST_ASSERT_EQUAL(static_cast<int>(PanPresence::PRESENT),
                    static_cast<int>(r.presence));
  TEST_ASSERT_FLOAT_WITHIN(6.0f, 200.0f, r.panTempC);
  TEST_ASSERT_FLOAT_WITHIN(2.0f, 16.0f, r.roiCx);
  TEST_ASSERT_FLOAT_WITHIN(2.0f, 12.0f, r.roiCy);
  TEST_ASSERT_TRUE(r.roiPixelCount >= MIN_PAN_PIXELS);
  TEST_ASSERT_TRUE(r.confidence > 0);
}

static void test_off_center_centroid(void) {
  FrameAnalyzer fa;
  PanReading r = fa.process(make_disc(24, 6, 4, 180.0f, 100));
  TEST_ASSERT_FLOAT_WITHIN(2.5f, 24.0f, r.roiCx);
  TEST_ASSERT_FLOAT_WITHIN(2.5f, 6.0f, r.roiCy);
}

static void test_no_pan_absent(void) {
  FrameAnalyzer fa;
  PanReading r = fa.process(make_uniform(AMB, 100));  // never present before
  TEST_ASSERT_EQUAL(static_cast<int>(PanPresence::ABSENT),
                    static_cast<int>(r.presence));
}

static void test_two_blobs_picks_larger(void) {
  FrameAnalyzer fa;
  ThermalFrame f = make_disc(8, 12, 5, 200.0f, 100);   // big blob left
  for (int r = 0; r < THERM_ROWS; ++r)                  // small blob right
    for (int c = 0; c < THERM_COLS; ++c)
      if (std::hypot(c - 26, r - 6) <= 2) f.px[r][c] = 210.0f;
  PanReading rr = fa.process(f);
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 8.0f, rr.roiCx);       // centroid on big blob
}

static void test_interior_percentile_not_max(void) {
  // Disc at 200 with a small 300 hotspot: p75 of interior ~200, well below max.
  FrameAnalyzer fa;
  ThermalFrame f = make_disc(16, 12, 6, 200.0f, 100);
  f.px[12][16] = 300.0f; f.px[12][17] = 300.0f; f.px[11][16] = 300.0f;
  PanReading r = fa.process(f);
  TEST_ASSERT_TRUE(r.panTempC < 260.0f);
  TEST_ASSERT_TRUE(r.panTempC > 180.0f);
  TEST_ASSERT_FLOAT_WITHIN(1.0f, 300.0f, r.roiMaxC);
}

static void test_edge_clipped_disc_detected(void) {
  FrameAnalyzer fa;
  PanReading r = fa.process(make_disc(1, 12, 5, 190.0f, 100));  // clipped left
  TEST_ASSERT_EQUAL(static_cast<int>(PanPresence::PRESENT),
                    static_cast<int>(r.presence));
  TEST_ASSERT_FLOAT_WITHIN(8.0f, 190.0f, r.panTempC);
}

static void test_obstruction_over_hot_pan(void) {
  FrameAnalyzer fa;
  // Establish a hot pan, then cover it with a warm (~34 °C) mass.
  TEST_ASSERT_EQUAL(int(PanPresence::PRESENT),
                    int(fa.process(make_disc(16, 12, 6, 200.0f, 100)).presence));
  PanReading r = fa.process(make_uniform(34.0f, 300));   // hand over the pan
  TEST_ASSERT_EQUAL(int(PanPresence::OBSTRUCTED), int(r.presence));
}

static void test_removed_pan_is_absent_not_obstructed(void) {
  FrameAnalyzer fa;
  fa.process(make_disc(16, 12, 6, 200.0f, 100));         // hot pan
  // Pan lifted away: scene returns to ambient, well past the absent window.
  PanReading r = fa.process(make_uniform(AMB, 100 + PRESENCE_ABSENT_MS + 500));
  TEST_ASSERT_EQUAL(int(PanPresence::ABSENT), int(r.presence));
}

static void test_cooling_pan_not_obstructed(void) {
  FrameAnalyzer fa;
  fa.process(make_disc(16, 12, 6, 200.0f, 100));         // hot pan
  // Still a warm pan disc (60 °C) — primary-detected, not a cover.
  PanReading r = fa.process(make_disc(16, 12, 6, 60.0f, 300));
  TEST_ASSERT_EQUAL(int(PanPresence::PRESENT), int(r.presence));
}

static void test_warm_mass_without_prior_pan(void) {
  FrameAnalyzer fa;
  // A warm mass with no pan ever tracked is not "obstruction".
  PanReading r = fa.process(make_uniform(34.0f, 100));
  TEST_ASSERT_NOT_EQUAL(int(PanPresence::OBSTRUCTED), int(r.presence));
}

static void test_stainless_signature(void) {
  // Low-ish reading with a big interior spread -> stainless hint + capped conf.
  FrameAnalyzer fa;
  ThermalFrame f = make_disc(16, 12, 6, 90.0f, 100);
  // scatter cool/hot noise inside to inflate spread (>25C) while temp stays low
  for (int r = 9; r <= 15; ++r)
    for (int c = 13; c <= 19; ++c)
      f.px[r][c] = ((r + c) & 1) ? 60.0f : 120.0f;
  PanReading rr = fa.process(f);
  TEST_ASSERT_TRUE(rr.stainlessHint);
  TEST_ASSERT_TRUE(rr.confidence <= STAINLESS_CONF_CAP);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_centered_disc_present);
  RUN_TEST(test_off_center_centroid);
  RUN_TEST(test_no_pan_absent);
  RUN_TEST(test_two_blobs_picks_larger);
  RUN_TEST(test_interior_percentile_not_max);
  RUN_TEST(test_edge_clipped_disc_detected);
  RUN_TEST(test_obstruction_over_hot_pan);
  RUN_TEST(test_removed_pan_is_absent_not_obstructed);
  RUN_TEST(test_cooling_pan_not_obstructed);
  RUN_TEST(test_warm_mass_without_prior_pan);
  RUN_TEST(test_stainless_signature);
  return UNITY_END();
}
