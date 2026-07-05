// i2c_bus.h — cross-core guard for the shared I2C bus (base spec §4). On the
// Advance board the GT911 touch (UITask core) and the MLX90640 (SensorTask core)
// live on the same SDA/SCL header, so every bus transaction takes this mutex to
// stop a 1.7 KB frame read colliding with a touch poll.
//
// On boards with a dedicated sensor bus (CAP_I2C_SHARED_TOUCH_SENSOR == 0) the
// lock is a cheap no-op holder but still compiled for a single code path.
#pragma once
#if !defined(PANPILOT_SIM)
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace hal {
void i2c_bus_init();
bool i2c_lock(uint32_t timeout_ms = 100);
void i2c_unlock();

// RAII helper.
struct I2CGuard {
  bool held;
  explicit I2CGuard(uint32_t to = 100) : held(i2c_lock(to)) {}
  ~I2CGuard() { if (held) i2c_unlock(); }
};
}  // namespace hal
#endif
