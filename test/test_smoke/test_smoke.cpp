// test_smoke — native build/sanity check so `pio test -e native` has a target
// from M0. Real core tests (thermal_model, frame_analysis, guidance) land with
// their modules (base spec §11). Also confirms the config headers compile host-side.
#include <unity.h>

#include "app_config.h"
#include "board_pins.h"

void setUp(void) {}
void tearDown(void) {}

static void test_arithmetic_sanity(void) { TEST_ASSERT_EQUAL_INT(4, 2 + 2); }

static void test_config_constants(void) {
  TEST_ASSERT_EQUAL_INT(4, LOGIC_TICK_HZ);      // base spec §4
  TEST_ASSERT_TRUE(I2C_FREQ_HZ >= 400000);      // §2.2 min for MLX frames
  TEST_ASSERT_TRUE(BACKLIGHT_IDLE_BRIGHT < BACKLIGHT_ACTIVE_BRIGHT);  // §8
}

static void test_board_sim_selected(void) {
  // Native/test env compiles under the BOARD_SIM block (no GPIOs).
  TEST_ASSERT_EQUAL_INT(0, CAP_DISPLAY_BUS_SPI);
  TEST_ASSERT_EQUAL_INT(0, CAP_DISPLAY_BUS_RGB);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_arithmetic_sanity);
  RUN_TEST(test_config_constants);
  RUN_TEST(test_board_sim_selected);
  return UNITY_END();
}
