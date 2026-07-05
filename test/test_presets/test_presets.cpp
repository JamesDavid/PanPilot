// test_presets — the built-in preset table is well-formed and maps to sane
// Target bands (base spec §7.3).
#include <unity.h>
#include "core/presets.cpp"  // include-the-TU (no guidance methods referenced)

void setUp() {}
void tearDown() {}

static void test_all_presets_wellformed(void) {
  for (uint8_t id = 0; id < PRESET_COUNT; ++id) {
    const Preset& p = preset(id);
    TEST_ASSERT_NOT_NULL(p.name);
    TEST_ASSERT_TRUE(p.loF < p.hiF);        // ready band ordered
    TEST_ASSERT_TRUE(p.hiF <= p.warnF);     // warn at/above the band top
    TEST_ASSERT_TRUE(p.warnF <= 650);       // within the hard ceiling (S5)
  }
}

static void test_preset_target_mapping(void) {
  Target t = preset_target(PRESET_SEAR);
  const Preset& p = preset(PRESET_SEAR);
  TEST_ASSERT_EQUAL_INT(p.loF, (int)t.loF);
  TEST_ASSERT_EQUAL_INT(p.hiF, (int)t.hiF);
  TEST_ASSERT_EQUAL_INT((p.loF + p.hiF) / 2, t.centerF);
}

static void test_stainless_flag(void) {
  TEST_ASSERT_TRUE(preset(PRESET_STAINLESS).stainlessHints);
  TEST_ASSERT_FALSE(preset(PRESET_EGGS).stainlessHints);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_all_presets_wellformed);
  RUN_TEST(test_preset_target_mapping);
  RUN_TEST(test_stainless_flag);
  return UNITY_END();
}
