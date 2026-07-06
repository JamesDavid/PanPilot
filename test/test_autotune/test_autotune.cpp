// test_autotune — the relay autotuner induces a clean oscillation on the same
// first-order plant the controller is tested against, converges to positive
// Ku/Tu and ZN gains, and those gains keep the plant bounded (roadmap §3.2).
#include <unity.h>
#include "core/control/autotune.cpp"
#include "core/control/controller.cpp"
#include <cmath>

namespace {
struct Plant {
  float t = 70, kHeat = 1.5f, kLoss = 0.004f, amb = 70;
  float step(float duty, float dt) {
    t += (kHeat * duty - kLoss * (t - amb)) * dt;
    return t;
  }
};
}  // namespace

void setUp() {}
void tearDown() {}

static void test_converges_to_positive_gains(void) {
  RelayAutotuner at;
  Plant p;
  at.start(/*setpoint*/ 350, /*dLow*/ 0.0f, /*dHigh*/ 1.0f, /*hyst*/ 1.0f, 0);
  float duty = 1.0f;
  uint32_t now = 0;
  for (int i = 0; i < 40000 && !at.converged(); ++i) {
    float temp = p.step(duty, 1.0f);
    now += 1000;                       // 1 s ticks
    duty = at.update(temp, now);
  }
  TEST_ASSERT_TRUE(at.converged());
  TEST_ASSERT_TRUE(at.ku() > 0.0f);
  TEST_ASSERT_TRUE(at.tuSeconds() > 0.0f);
  TEST_ASSERT_TRUE(at.kp() > 0.0f);
  TEST_ASSERT_TRUE(at.ki() > 0.0f);
  TEST_ASSERT_TRUE(at.kd() > 0.0f);
}

static void test_tuned_gains_keep_plant_bounded(void) {
  RelayAutotuner at;
  Plant p;
  at.start(350, 0.0f, 1.0f, 1.0f, 0);
  float duty = 1.0f;
  uint32_t now = 0;
  for (int i = 0; i < 40000 && !at.converged(); ++i) {
    float temp = p.step(duty, 1.0f);
    now += 1000;
    duty = at.update(temp, now);
  }
  TEST_ASSERT_TRUE(at.converged());

  Controller c;
  c.setLaw(ControlLaw::PID);
  c.setGains(at.kp(), at.ki(), at.kd());
  c.reset();
  Plant q;                              // fresh plant, PID from the tuned gains
  float d = 0;
  for (int i = 0; i < 8000; ++i) {
    float temp = q.step(d, 1.0f);
    d = c.update(temp, 350, 1.0f);
  }
  TEST_ASSERT_FLOAT_WITHIN(50.0f, 350.0f, q.t);   // stable, not diverging
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_converges_to_positive_gains);
  RUN_TEST(test_tuned_gains_keep_plant_bounded);
  return UNITY_END();
}
