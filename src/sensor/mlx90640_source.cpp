// mlx90640_source.cpp — see mlx90640_source.h. Device-only.
#if !defined(PANPILOT_SIM)
#include "mlx90640_source.h"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MLX90640.h>

#include "board_pins.h"
#include "app_config.h"
#include "hal/i2c_bus.h"

namespace sensor {
namespace {
Adafruit_MLX90640 s_mlx;
float s_frame[THERM_PIXELS];
int   s_badFrames = 0;
float s_dieC = 25.0f;
bool  s_ready = false;

// The MLX shares SDA/SCL with the GT911 touch (LovyanGFX owns I2C port 0), so
// the sensor uses the second controller (Wire1) on the same header pins; the
// hal::i2c_bus mutex serialises the two so only one transacts at a time.
// >>> Bus arrangement is the #1 M1 bench-verify item (HARDWARE_TEST.md): if
// >>> touch and sensor interfere, move the MLX to dedicated GPIO_D pins.
TwoWire& busWire() { return Wire1; }
}  // namespace

bool mlx_begin() {
  hal::i2c_bus_init();
  busWire().begin(I2C_SDA, I2C_SCL, I2C_FREQ_HZ);
  hal::I2CGuard g;
  if (!s_mlx.begin(MLX90640_I2C_ADDR, &busWire())) return false;
  s_mlx.setMode(MLX90640_CHESS);
  s_mlx.setResolution(MLX90640_ADC_18BIT);
  s_mlx.setRefreshRate(MLX90640_8_HZ);
  // Emissivity default is applied by the driver's calc; per-profile override
  // (base spec §2.2) is wired when profiles land (M6).
  s_ready = true;
  s_badFrames = 0;
  return true;
}

bool mlx_read(ThermalFrame& out) {
  if (!s_ready) { out.valid = false; return false; }
  int rc;
  {
    hal::I2CGuard g(120);
    if (!g.held) { out.valid = false; return false; }
    rc = s_mlx.getFrame(s_frame);   // reads both subpages
  }
  if (rc != 0) {
    if (++s_badFrames >= SENSOR_BAD_FRAMES_FAULT) { /* fault sticky */ }
    out.valid = false;
    return false;
  }

  // Validate + copy (reject NaN/inf/out-of-range, base spec §5).
  float lo = 1e9f;
  for (int r = 0; r < THERM_ROWS; ++r)
    for (int c = 0; c < THERM_COLS; ++c) {
      const float v = s_frame[r * THERM_COLS + c];
      if (!(v > SENSOR_MIN_VALID_C && v < SENSOR_MAX_VALID_C)) {
        if (++s_badFrames >= SENSOR_BAD_FRAMES_FAULT) {}
        out.valid = false;
        return false;
      }
      out.px[r][c] = v;
      if (v < lo) lo = v;
    }
  s_badFrames = 0;
  s_dieC = lo;                     // proxy die/ambient (refine w/ GetTa later)
  out.ambientC = lo;
  out.t_ms = millis();
  out.valid = true;
  return true;
}

bool  mlx_faulted() { return s_badFrames >= SENSOR_BAD_FRAMES_FAULT; }
float mlx_die_tempC() { return s_dieC; }

}  // namespace sensor
#endif
