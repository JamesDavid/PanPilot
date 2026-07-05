// mlx90640_source.h — MLX90640 acquisition (base spec §5). Device-only; the
// simulator feeds synthetic frames instead. Assembles full frames (chess mode,
// 8 Hz subpages), rejects out-of-range pixels, tracks a bad-frame fault, and
// exposes the sensor die temperature for the over-temp warning.
#pragma once
#if !defined(PANPILOT_SIM)
#include "pan_types.h"

namespace sensor {

// Probe + init on the shared I2C bus. false => sensor not found (caller shows
// the "SENSOR NOT FOUND — check cable" screen and retries, §5).
bool mlx_begin();

// Read one full frame into `out`. false on a bad/rejected read. Takes the I2C
// bus lock internally (hal::i2c_bus).
bool mlx_read(ThermalFrame& out);

bool  mlx_faulted();       // true after SENSOR_BAD_FRAMES_FAULT consecutive bad
float mlx_die_tempC();     // last ambient/die temperature

}  // namespace sensor
#endif
