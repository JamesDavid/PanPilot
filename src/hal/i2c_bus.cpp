// i2c_bus.cpp — see i2c_bus.h.
#if !defined(PANPILOT_SIM)
#include "i2c_bus.h"

namespace hal {
namespace {
SemaphoreHandle_t s_mutex = nullptr;
}

void i2c_bus_init() {
  if (!s_mutex) s_mutex = xSemaphoreCreateMutex();
}
bool i2c_lock(uint32_t timeout_ms) {
  if (!s_mutex) i2c_bus_init();
  return xSemaphoreTake(s_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}
void i2c_unlock() {
  if (s_mutex) xSemaphoreGive(s_mutex);
}
}  // namespace hal
#endif
