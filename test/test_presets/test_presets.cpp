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

// --- custom presets (Phase 2 editor) ---
static void test_custom_add_update_remove(void) {
  presets_load_custom(nullptr, 0);                 // reset
  TEST_ASSERT_EQUAL_INT(PRESET_COUNT, presets_total());

  const int id = presets_add("Smash", 450, 500, false);
  TEST_ASSERT_EQUAL_INT(PRESET_COUNT, id);         // first custom id
  TEST_ASSERT_EQUAL_INT(PRESET_COUNT + 1, presets_total());
  TEST_ASSERT_TRUE(presets_is_custom(id));
  const Preset& p = preset(id);
  TEST_ASSERT_EQUAL_STRING("Smash", p.name);
  TEST_ASSERT_EQUAL_INT(450, p.loF);
  TEST_ASSERT_EQUAL_INT(600, p.warnF);             // hi (500) + 100
  TEST_ASSERT_TRUE(p.recoveryMonitor);             // user presets monitor recovery

  presets_update(id, "Smash+", 460, 520, true);
  TEST_ASSERT_EQUAL_STRING("Smash+", preset(id).name);
  TEST_ASSERT_TRUE(preset(id).stainlessHints);
  TEST_ASSERT_EQUAL_INT(620, preset(id).warnF);

  presets_remove(id);
  TEST_ASSERT_EQUAL_INT(PRESET_COUNT, presets_total());
}

static void test_custom_reject_and_cap(void) {
  presets_load_custom(nullptr, 0);
  TEST_ASSERT_EQUAL_INT(-1, presets_add("bad", 400, 400, false));  // hi<=lo
  for (int i = 0; i < PRESET_CUSTOM_MAX; ++i)
    TEST_ASSERT_TRUE(presets_add("c", 300, 350, false) >= 0);
  TEST_ASSERT_EQUAL_INT(-1, presets_add("overflow", 300, 350, false));  // full
  TEST_ASSERT_EQUAL_INT(450, preset(PRESET_COUNT).warnF);  // 350 + 100
  presets_load_custom(nullptr, 0);
}

static void test_custom_warn_clamped(void) {
  presets_load_custom(nullptr, 0);
  const int id = presets_add("Inferno", 560, 600, false);   // 600 + 100 -> clamp
  TEST_ASSERT_EQUAL_INT(650, preset(id).warnF);
  presets_load_custom(nullptr, 0);
}

static void test_custom_blob_roundtrip(void) {
  presets_load_custom(nullptr, 0);
  presets_add("Roti", 400, 450, false);
  presets_add("Blackening", 500, 560, true);
  // snapshot the blob
  static uint8_t buf[PRESET_CUSTOM_MAX * 32];
  const uint32_t n = presets_custom_blob_bytes();
  memcpy(buf, presets_custom_blob(), n);
  presets_load_custom(nullptr, 0);
  TEST_ASSERT_EQUAL_INT(PRESET_COUNT, presets_total());
  presets_load_custom(buf, n);
  TEST_ASSERT_EQUAL_INT(PRESET_COUNT + 2, presets_total());
  TEST_ASSERT_EQUAL_STRING("Blackening", preset(PRESET_COUNT + 1).name);
  TEST_ASSERT_TRUE(preset(PRESET_COUNT + 1).stainlessHints);
  presets_load_custom(nullptr, 0);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_all_presets_wellformed);
  RUN_TEST(test_preset_target_mapping);
  RUN_TEST(test_stainless_flag);
  RUN_TEST(test_custom_add_update_remove);
  RUN_TEST(test_custom_reject_and_cap);
  RUN_TEST(test_custom_warn_clamped);
  RUN_TEST(test_custom_blob_roundtrip);
  return UNITY_END();
}
