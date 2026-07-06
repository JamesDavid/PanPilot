// test_controller — bang-bang and PID hold a simulated first-order plant near
// setpoint (roadmap §3.2, M16/M17 acceptance).
#include <unity.h>
#include "core/control/controller.cpp"
#include <cmath>

namespace {
// First-order plant: dT = (kHeat*duty - kLoss*(T-amb)) dt. Time constant minutes.
struct Plant {
  float t = 70, kHeat = 1.5f, kLoss = 0.004f, amb = 70;
  float step(float duty, float dt) {
    t += (kHeat * duty - kLoss * (t - amb)) * dt;
    return t;
  }
};
float run(Controller& c, int ticks) {
  Plant p; float duty = 0;
  for (int i = 0; i < ticks; ++i) { float temp = p.step(duty, 1.0f); duty = c.update(temp, 350, 1.0f); }
  return p.t;
}
}  // namespace

void setUp() {}
void tearDown() {}

static void test_bangbang_holds(void) {
  Controller c; c.setLaw(ControlLaw::BANGBANG); c.setBand(3);
  TEST_ASSERT_FLOAT_WITHIN(15.0f, 350.0f, run(c, 4000));   // ±15 (M16)
}

static void test_pid_holds_tighter(void) {
  Controller c; c.setLaw(ControlLaw::PID); c.setGains(0.02f, 0.0008f, 0.02f);
  c.reset();
  TEST_ASSERT_FLOAT_WITHIN(10.0f, 350.0f, run(c, 6000));   // ±10 (M17)
}

static void test_duty_clamped(void) {
  Controller c; c.setLaw(ControlLaw::PID); c.setGains(1, 1, 0); c.reset();
  float d = c.update(0, 350, 1.0f);       // huge error
  TEST_ASSERT_TRUE(d <= 1.0f && d >= 0.0f);
  d = c.update(700, 350, 1.0f);           // way over
  TEST_ASSERT_TRUE(d <= 1.0f && d >= 0.0f);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_bangbang_holds);
  RUN_TEST(test_pid_holds_tighter);
  RUN_TEST(test_duty_clamped);
  return UNITY_END();
}
