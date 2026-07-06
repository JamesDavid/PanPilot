// lgfx_panpilot_rgb.h — LovyanGFX device for the 800x480 RGB-parallel ST7262
// CrowPanel Advance 5" boards (v1.1 / v1.2). Uses LovyanGFX's native esp32-S3
// Bus_RGB + Panel_RGB (framebuffer in PSRAM) — the LovyanGFX-native path, not
// the esp_lcd/neutered-Bus_RGB approach in ../BladeKey-Overhead. All pins and
// timings come from board_pins.h.
//
// BENCH-GATED: sync polarity, pclk edge (pclk_active_neg / pclk_idle_high), and
// the I/O-expander reset/backlight sequence must be verified on the physical
// panel — flip them per HARDWARE_TEST if the panel stays black. Compile-verified
// only until then.
#pragma once
#include "board_pins.h"

#if CAP_DISPLAY_BUS_RGB

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
// Bus_RGB/Panel_RGB are not pulled in by <LovyanGFX.hpp> — include explicitly.
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

class LGFX_PanPilotRGB : public lgfx::LGFX_Device {
  lgfx::Bus_RGB     _bus;
  lgfx::Panel_RGB   _panel;
  lgfx::Touch_GT911 _touch;

 public:
  LGFX_PanPilotRGB() {
    { // Panel geometry (native landscape 800x480), framebuffer in PSRAM.
      auto cfg = _panel.config();
      cfg.memory_width  = TFT_PANEL_WIDTH;
      cfg.memory_height = TFT_PANEL_HEIGHT;
      cfg.panel_width   = TFT_PANEL_WIDTH;
      cfg.panel_height  = TFT_PANEL_HEIGHT;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      _panel.config(cfg);

      auto det = _panel.config_detail();
      det.use_psram = 1;
      _panel.config_detail(det);
    }
    { // 16-bit RGB565 parallel bus. LovyanGFX pin_d0..d15 = B0..B4,G0..G5,R0..R4.
      auto cfg = _bus.config();
      cfg.panel = &_panel;
      cfg.pin_d0  = RGB_PIN_B0; cfg.pin_d1  = RGB_PIN_B1; cfg.pin_d2  = RGB_PIN_B2;
      cfg.pin_d3  = RGB_PIN_B3; cfg.pin_d4  = RGB_PIN_B4;
      cfg.pin_d5  = RGB_PIN_G0; cfg.pin_d6  = RGB_PIN_G1; cfg.pin_d7  = RGB_PIN_G2;
      cfg.pin_d8  = RGB_PIN_G3; cfg.pin_d9  = RGB_PIN_G4; cfg.pin_d10 = RGB_PIN_G5;
      cfg.pin_d11 = RGB_PIN_R0; cfg.pin_d12 = RGB_PIN_R1; cfg.pin_d13 = RGB_PIN_R2;
      cfg.pin_d14 = RGB_PIN_R3; cfg.pin_d15 = RGB_PIN_R4;
      cfg.pin_henable = RGB_PIN_DE;
      cfg.pin_vsync   = RGB_PIN_VSYNC;
      cfg.pin_hsync   = RGB_PIN_HSYNC;
      cfg.pin_pclk    = RGB_PIN_PCLK;
      cfg.freq_write  = RGB_PCLK_HZ;

      cfg.hsync_polarity    = 0;        // BENCH-GATED
      cfg.hsync_front_porch = RGB_HSYNC_FRONT_PORCH;
      cfg.hsync_pulse_width = RGB_HSYNC_PULSE_WIDTH;
      cfg.hsync_back_porch  = RGB_HSYNC_BACK_PORCH;
      cfg.vsync_polarity    = 0;        // BENCH-GATED
      cfg.vsync_front_porch = RGB_VSYNC_FRONT_PORCH;
      cfg.vsync_pulse_width = RGB_VSYNC_PULSE_WIDTH;
      cfg.vsync_back_porch  = RGB_VSYNC_BACK_PORCH;
      cfg.pclk_active_neg   = 1;        // BENCH-GATED (flip if panel stays black)
      cfg.pclk_idle_high    = (RGB_PCLK_IDLE_HIGH != 0);
      cfg.de_idle_high      = 0;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    { // GT911 capacitive touch (I2C, shares the peripheral bus with the MLX90640)
      auto cfg = _touch.config();
      cfg.x_min = 0; cfg.x_max = TFT_PANEL_WIDTH - 1;
      cfg.y_min = 0; cfg.y_max = TFT_PANEL_HEIGHT - 1;
      cfg.pin_int = TOUCH_INT;
      cfg.pin_rst = TOUCH_RST;          // -1: reset is pulsed via the expander
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      cfg.i2c_port = 0;
      cfg.pin_sda = TOUCH_SDA;
      cfg.pin_scl = TOUCH_SCL;
      cfg.freq = 400000;
      cfg.i2c_addr = TOUCH_I2C_ADDR;
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
  }
};

#endif  // CAP_DISPLAY_BUS_RGB
