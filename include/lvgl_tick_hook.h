// lvgl_tick_hook.h — millisecond source for LVGL's custom tick (lv_conf.h).
// Device: Arduino millis(). Simulator/native: std::chrono steady clock.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t panpilot_millis(void);

#ifdef __cplusplus
}
#endif
