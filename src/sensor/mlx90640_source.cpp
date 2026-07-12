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
// the sensor uses the second controller (Wire1); the hal::i2c_bus mutex
// serialises the two so only one transacts at a time.
TwoWire& busWire() { return Wire1; }

int s_sda = I2C_SDA, s_scl = I2C_SCL;   // pins the sensor was FOUND on

// Re-route the controller onto the pins before EVERY transaction block:
// arduino-esp32's Wire.begin() is a no-op once initialized, and the GT911
// (LovyanGFX's own i2c layer) re-grabs the GPIO matrix for its polls — bench-
// found 2026-07-12: without the end()+begin(), Wire1 toggled a disconnected
// controller and the MLX never ACKed even with correct wiring.
void grabBus(int sda, int scl) {
  busWire().end();
  busWire().begin(sda, scl, I2C_FREQ_HZ);
  busWire().setTimeOut(50);   // ms/op — a stuck bus must not stall the guard
}
}  // namespace

bool mlx_begin() {
  hal::i2c_bus_init();
  hal::I2CGuard g;
  // Try the I2C-OUT header first (shared with touch), then the UART1-OUT
  // header repurposed as a dedicated bus (both wire orders) — the module
  // works plugged into either connector.
  static const struct { int sda, scl; const char* where; } kCand[] = {
      {I2C_SDA, I2C_SCL, "I2C-OUT (shared with touch)"},
      {I2C_ALT_A, I2C_ALT_B, "UART1-OUT (dedicated)"},
      {I2C_ALT_B, I2C_ALT_A, "UART1-OUT (dedicated, swapped)"},
  };
  for (const auto& c : kCand) {
    grabBus(c.sda, c.scl);
    if (s_mlx.begin(MLX90640_I2C_ADDR, &busWire())) {
      s_sda = c.sda;
      s_scl = c.scl;
      Serial.printf("[mlx] MLX90640 found on %s  SDA=%d SCL=%d\n",
                    c.where, c.sda, c.scl);
      s_mlx.setMode(MLX90640_CHESS);
      s_mlx.setResolution(MLX90640_ADC_18BIT);
      s_mlx.setRefreshRate(MLX90640_8_HZ);
      // Emissivity default is applied by the driver's calc; per-profile
      // override (base spec §2.2) is wired when profiles land (M6).
      s_ready = true;
      s_badFrames = 0;
      return true;
    }
  }
  // Bench diagnostic, BOUNDED (a full 127-address scan once wedged for
  // minutes on a stuck bus while holding the I2C mutex — froze touch).
  static uint32_t lastDiag = 0;
  if (millis() - lastDiag > 10000) {
    lastDiag = millis();
    grabBus(I2C_SDA, I2C_SCL);
    Serial.printf("[mlx] no ACK @0x%02X on any header  SDA(io%d)=%d SCL(io%d)=%d\n",
                  MLX90640_I2C_ADDR, I2C_SDA, digitalRead(I2C_SDA), I2C_SCL,
                  digitalRead(I2C_SCL));
  }
  return false;
}

bool mlx_read(ThermalFrame& out) {
  if (!s_ready) { out.valid = false; return false; }
  int rc;
  {
    hal::I2CGuard g(120);
    if (!g.held) { out.valid = false; return false; }
    grabBus(s_sda, s_scl);          // touch polls re-route the pins between reads
    rc = s_mlx.getFrame(s_frame);   // reads both subpages
  }
  if (rc != 0) {
    if (++s_badFrames >= SENSOR_BAD_FRAMES_FAULT) { /* fault sticky */ }
    out.valid = false;
    return false;
  }

  // Validate + copy (reject NaN/inf/out-of-range, base spec §5), applying the
  // SENSOR_FLIP_* orientation so every consumer sees a camera-style frame.
  float lo = 1e9f;
  for (int r = 0; r < THERM_ROWS; ++r)
    for (int c = 0; c < THERM_COLS; ++c) {
      const int sr = SENSOR_FLIP_Y ? (THERM_ROWS - 1 - r) : r;
      const int sc = SENSOR_FLIP_X ? (THERM_COLS - 1 - c) : c;
      const float v = s_frame[sr * THERM_COLS + sc];
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
