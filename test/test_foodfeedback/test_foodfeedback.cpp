// test_foodfeedback — post-cook ±8% personalization: direction, compounding,
// clamps, per-(name,variant) isolation, PERFECT no-op, blob round-trip (§2.7).
#include <unity.h>
#include "core/foodfeedback.h"

using panpilot::FeedbackStore;
using V = panpilot::FeedbackStore::Verdict;

void setUp() {}
void tearDown() {}

static void test_default_is_unity(void) {
  FeedbackStore s;
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 1.0f, s.factorFor("Pancakes", "4-inch"));
}

static void test_under_lengthens_over_shortens(void) {
  FeedbackStore s;
  s.apply("Pancakes", "4-inch", V::UNDER);
  TEST_ASSERT_FLOAT_WITHIN(1e-5, 1.08f, s.factorFor("Pancakes", "4-inch"));
  FeedbackStore t;
  t.apply("Pancakes", "4-inch", V::OVER);
  TEST_ASSERT_FLOAT_WITHIN(1e-5, 0.92f, t.factorFor("Pancakes", "4-inch"));
}

static void test_compounds(void) {
  FeedbackStore s;
  s.apply("Steak", "1-inch", V::UNDER);
  s.apply("Steak", "1-inch", V::UNDER);
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 1.08f * 1.08f, s.factorFor("Steak", "1-inch"));
}

static void test_perfect_is_noop(void) {
  FeedbackStore s;
  s.apply("Eggs", "over easy", V::OVER);          // -> 0.92
  s.apply("Eggs", "over easy", V::PERFECT);        // keep
  TEST_ASSERT_FLOAT_WITHIN(1e-5, 0.92f, s.factorFor("Eggs", "over easy"));
  TEST_ASSERT_EQUAL_INT(1, s.count());
}

static void test_clamps(void) {
  FeedbackStore s;
  for (int i = 0; i < 20; ++i) s.apply("A", "b", V::UNDER);
  TEST_ASSERT_FLOAT_WITHIN(1e-5, FeedbackStore::HI, s.factorFor("A", "b"));
  FeedbackStore t;
  for (int i = 0; i < 20; ++i) t.apply("A", "b", V::OVER);
  TEST_ASSERT_FLOAT_WITHIN(1e-5, FeedbackStore::LO, t.factorFor("A", "b"));
}

static void test_variants_isolated(void) {
  FeedbackStore s;
  s.apply("Pancakes", "4-inch", V::UNDER);
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 1.0f, s.factorFor("Pancakes", "6-inch"));
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 1.0f, s.factorFor("Bacon", "4-inch"));
}

static void test_blob_roundtrip(void) {
  FeedbackStore s;
  s.apply("Pancakes", "4-inch", V::UNDER);
  s.apply("Bacon", "thick cut", V::OVER);
  FeedbackStore t;
  t.loadBlob(s.blob(), s.blobBytes());
  TEST_ASSERT_EQUAL_INT(2, t.count());
  TEST_ASSERT_FLOAT_WITHIN(1e-5, 1.08f, t.factorFor("Pancakes", "4-inch"));
  TEST_ASSERT_FLOAT_WITHIN(1e-5, 0.92f, t.factorFor("Bacon", "thick cut"));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_default_is_unity);
  RUN_TEST(test_under_lengthens_over_shortens);
  RUN_TEST(test_compounds);
  RUN_TEST(test_perfect_is_noop);
  RUN_TEST(test_clamps);
  RUN_TEST(test_variants_isolated);
  RUN_TEST(test_blob_roundtrip);
  return UNITY_END();
}
