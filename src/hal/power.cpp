// power.cpp — see power.h. Raw MAX17048 reads on the shared bus (Wire1).
#if !defined(PANPILOT_SIM)
#include "power.h"

#include <Arduino.h>
#include <Wire.h>

#include "board_pins.h"
#include "app_config.h"
#include "hal/i2c_bus.h"

#ifndef BATTERY_USB_DETECT_PIN
#define BATTERY_USB_DETECT_PIN -1
#endif

namespace hal {
namespace {
bool s_gauge = false;

bool readReg16(uint8_t reg, uint16_t& out) {
  Wire1.beginTransmission(MAX17048_I2C_ADDR);
  Wire1.write(reg);
  if (Wire1.endTransmission(false) != 0) return false;
  if (Wire1.requestFrom((int)MAX17048_I2C_ADDR, 2) != 2) return false;
  const uint8_t hi = Wire1.read(), lo = Wire1.read();
  out = ((uint16_t)hi << 8) | lo;
  return true;
}
}  // namespace

void power_begin() {
  hal::i2c_bus_init();
  Wire1.begin(I2C_SDA, I2C_SCL, I2C_FREQ_HZ);
  hal::I2CGuard g;
  uint16_t v;
  s_gauge = readReg16(0x02, v);   // VCELL — probes presence
#if BATTERY_USB_DETECT_PIN >= 0
  pinMode(BATTERY_USB_DETECT_PIN, INPUT);
#endif
}

BatteryState power_read() {
  BatteryState s;
#if BATTERY_USB_DETECT_PIN >= 0
  s.usbPresent = digitalRead(BATTERY_USB_DETECT_PIN) != 0;
#else
  s.usbPresent = true;            // no detect pin -> assume USB (safe default)
#endif
  if (s_gauge) {
    hal::I2CGuard g(30);
    uint16_t soc;
    if (g.held && readReg16(0x04, soc)) {   // SOC reg: hi byte = integer %
      s.soc = (uint8_t)((soc >> 8) & 0xFF);
      if (s.soc > 100) s.soc = 100;
      s.valid = true;
      // On battery, a rising SoC while unplugged is implausible; treat any USB
      // detect as charging (real charge status needs the charger IC).
      s.charging = s.usbPresent;
    }
  }
  return s;
}

}  // namespace hal
#endif
