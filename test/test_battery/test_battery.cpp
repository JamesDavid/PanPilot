// test_battery — low/critical edge events, USB suppression, re-arm hysteresis
// (roadmap spec §2.1).
#include <unity.h>
#include "core/battery.cpp"

namespace {
BatteryState st(uint8_t soc, bool usb, bool valid = true) {
  BatteryState s; s.soc = soc; s.usbPresent = usb; s.valid = valid; return s;
}
}  // namespace

void setUp() {}
void tearDown() {}

static void test_low_then_critical_fire_once(void) {
  BatteryMonitor m;
  TEST_ASSERT_EQUAL(int(BattEvent::NONE), int(m.update(st(50, false))));
  TEST_ASSERT_EQUAL(int(BattEvent::LOW_BATT), int(m.update(st(14, false))));
  TEST_ASSERT_EQUAL(int(BattEvent::NONE), int(m.update(st(13, false))));  // no repeat
  TEST_ASSERT_EQUAL(int(BattEvent::CRITICAL), int(m.update(st(4, false))));
  TEST_ASSERT_EQUAL(int(BattEvent::NONE), int(m.update(st(3, false))));   // no repeat
}

static void test_usb_suppresses(void) {
  BatteryMonitor m;
  TEST_ASSERT_EQUAL(int(BattEvent::NONE), int(m.update(st(4, /*usb*/ true))));
}

static void test_rearm_after_recovery(void) {
  BatteryMonitor m;
  m.update(st(14, false));                        // LOW fired
  m.update(st(30, false));                        // recovered -> re-arm
  TEST_ASSERT_EQUAL(int(BattEvent::LOW_BATT), int(m.update(st(14, false))));  // fires again
}

static void test_invalid_no_event(void) {
  BatteryMonitor m;
  TEST_ASSERT_EQUAL(int(BattEvent::NONE), int(m.update(st(2, false, /*valid*/ false))));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_low_then_critical_fire_once);
  RUN_TEST(test_usb_suppresses);
  RUN_TEST(test_rearm_after_recovery);
  RUN_TEST(test_invalid_no_event);
  return UNITY_END();
}
