// test_multipan — top-2 blob detection + persistent zone IDs (roadmap §2.4).
#include <unity.h>
#include <cmath>
#include "core/multipan.cpp"

namespace {
void disc(ThermalFrame& f, float cx, float cy, float rad, float hot) {
  for (int r = 0; r < THERM_ROWS; ++r)
    for (int c = 0; c < THERM_COLS; ++c)
      if (std::hypot(c - cx, r - cy) <= rad) f.px[r][c] = hot;
}
ThermalFrame two(float cx0, float cy0, float r0, float cx1, float cy1, float r1) {
  ThermalFrame f{}; f.valid = true; f.ambientC = 25;
  for (int r = 0; r < THERM_ROWS; ++r)
    for (int c = 0; c < THERM_COLS; ++c) f.px[r][c] = 25;
  disc(f, cx0, cy0, r0, 200);
  disc(f, cx1, cy1, r1, 200);
  return f;
}
}  // namespace

void setUp() {}
void tearDown() {}

static void test_detects_two(void) {
  MultiPanTracker t; PanReading o[2];
  int n = t.process(two(8, 12, 4, 24, 10, 3), o);
  TEST_ASSERT_EQUAL(2, n);
  TEST_ASSERT_EQUAL(int(PanPresence::PRESENT), int(o[0].presence));
  TEST_ASSERT_EQUAL(int(PanPresence::PRESENT), int(o[1].presence));
}

static void test_single_leaves_zone1_absent(void) {
  MultiPanTracker t; PanReading o[2];
  int n = t.process(two(16, 12, 5, 16, 12, 0.0f), o);  // second disc radius 0
  TEST_ASSERT_EQUAL(1, n);
  TEST_ASSERT_EQUAL(int(PanPresence::ABSENT), int(o[1].presence));
}

static void test_persistent_ids_survive_size_swap(void) {
  MultiPanTracker t; PanReading o[2];
  // Frame 1: big pan at col 8, small pan at col 24.
  t.process(two(8, 12, 5, 24, 12, 3), o);
  const float z0 = o[0].roiCx, z1 = o[1].roiCx;
  TEST_ASSERT_FLOAT_WITHIN(2.5f, 8.0f, z0);
  TEST_ASSERT_FLOAT_WITHIN(2.5f, 24.0f, z1);
  // Frame 2: sizes swap (col 24 now big, col 8 small) but positions unchanged.
  t.process(two(8, 12, 3, 24, 12, 5), o);
  TEST_ASSERT_FLOAT_WITHIN(2.5f, 8.0f, o[0].roiCx);   // zone 0 stays the col-8 pan
  TEST_ASSERT_FLOAT_WITHIN(2.5f, 24.0f, o[1].roiCx);  // zone 1 stays the col-24 pan
}

static void test_split_blob_is_one_pan_not_two(void) {
  // A spatula across one pan momentarily splits its blob into two fragments a
  // few px apart — that must NOT activate a phantom second zone.
  MultiPanTracker t; PanReading o[2];
  ThermalFrame f{}; f.valid = true; f.ambientC = 25;
  for (int r = 0; r < THERM_ROWS; ++r)
    for (int c = 0; c < THERM_COLS; ++c) f.px[r][c] = 25;
  disc(f, 14, 12, 3, 200);            // two fragments ~4.5 px apart (one pan)
  disc(f, 18.5f, 12, 2.2f, 200);
  int n = t.process(f, o);
  TEST_ASSERT_EQUAL(1, n);
  TEST_ASSERT_EQUAL(int(PanPresence::ABSENT), int(o[1].presence));
}

static void test_one_frame_miss_keeps_zone_identity(void) {
  MultiPanTracker t; PanReading o[2];
  t.process(two(8, 12, 5, 24, 12, 3), o);            // both pans tracked
  // Pan B (col 24) drops out for ONE noisy frame...
  t.process(two(8, 12, 5, 24, 12, 0.0f), o);
  TEST_ASSERT_EQUAL(int(PanPresence::ABSENT), int(o[1].presence));
  // ...and returns: it must still be zone 1 (identity survived the flicker,
  // not reshuffled into a "new pan" slot).
  t.process(two(8, 12, 5, 24, 12, 3), o);
  TEST_ASSERT_FLOAT_WITHIN(2.5f, 24.0f, o[1].roiCx);
  TEST_ASSERT_FLOAT_WITHIN(2.5f, 8.0f, o[0].roiCx);
}

static void test_zone2_stainless_signature(void) {
  // Zone 2 gets the same reflective-stainless flag as the primary analyzer —
  // without it the stainless TOO_HOT suppression never applied to pan 2.
  MultiPanTracker t; PanReading o[2];
  ThermalFrame f = two(8, 12, 5, 24, 12, 4);
  for (int r = 9; r <= 15; ++r)                       // noisy low-temp interior
    for (int c = 21; c <= 27; ++c)
      if (std::hypot(c - 24, r - 12) <= 4) f.px[r][c] = ((r + c) & 1) ? 60 : 120;
  t.process(f, o);
  TEST_ASSERT_TRUE(o[1].stainlessHint);
  TEST_ASSERT_TRUE(o[1].confidence <= STAINLESS_CONF_CAP);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_detects_two);
  RUN_TEST(test_single_leaves_zone1_absent);
  RUN_TEST(test_persistent_ids_survive_size_swap);
  RUN_TEST(test_split_blob_is_one_pan_not_two);
  RUN_TEST(test_one_frame_miss_keeps_zone_identity);
  RUN_TEST(test_zone2_stainless_signature);
  return UNITY_END();
}
