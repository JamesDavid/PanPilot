// lgfx_panpilot.h — LovyanGFX device for the CrowPanel Advance 3.5" (SPI ILI9488
// + integrated GT911 touch). Config mirrors Elecrow's factory LovyanGFX driver
// (lesson-03/3_5LVGL/LovyanGFX_Driver.h), with all pins sourced from board_pins.h.
//
// Guarded to SPI-ILI9488 + GT911 boards. RGB-parallel 5" targets get their own
// device class when that HAL path is built (see board_pins.h CAP_DISPLAY_BUS_RGB).
#pragma once
#include "board_pins.h"

#if CAP_DISPLAY_BUS_SPI && CAP_TOUCH_GT911

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX_PanPilot : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9488 _panel;
  lgfx::Bus_SPI       _bus;
  lgfx::Light_PWM     _light;
  lgfx::Touch_GT911   _touch;

 public:
  LGFX_PanPilot() {
    { // SPI bus (SPI2_HOST, 40 MHz write) — factory values
      auto cfg = _bus.config();
      cfg.spi_host    = SPI2_HOST;
      cfg.spi_mode    = 0;
      cfg.freq_write  = 40000000;
      cfg.freq_read   = 16000000;
      cfg.spi_3wire   = false;
      cfg.use_lock    = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk    = TFT_SCLK;
      cfg.pin_mosi    = TFT_MOSI;
      cfg.pin_miso    = TFT_MISO;
      cfg.pin_dc      = TFT_DC;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    { // Panel ILI9488
      auto cfg = _panel.config();
      cfg.pin_cs           = TFT_CS;
      cfg.pin_rst          = TFT_RST;
      cfg.pin_busy         = -1;
      cfg.memory_width     = TFT_PANEL_WIDTH;
      cfg.memory_height    = TFT_PANEL_HEIGHT;
      cfg.panel_width      = TFT_PANEL_WIDTH;
      cfg.panel_height     = TFT_PANEL_HEIGHT;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = false;
      cfg.invert           = (TFT_INVERT != 0);
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = (TFT_BUS_SHARED != 0);
      _panel.config(cfg);
    }
    { // Backlight (PWM via LEDC)
      auto cfg = _light.config();
      cfg.pin_bl      = TFT_BL;
      cfg.invert      = false;
      cfg.freq        = 5000;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    { // Touch GT911 over I2C (shares the peripheral bus with the MLX90640)
      auto cfg = _touch.config();
      cfg.x_min          = 0;
      cfg.x_max          = TFT_PANEL_WIDTH - 1;
      cfg.y_min          = 0;
      cfg.y_max          = TFT_PANEL_HEIGHT - 1;
      cfg.pin_int        = TOUCH_INT;
      cfg.pin_rst        = TOUCH_RST;
      cfg.bus_shared     = false;
      cfg.offset_rotation = 0;
      cfg.i2c_port       = 0;
      cfg.pin_sda        = TOUCH_SDA;
      cfg.pin_scl        = TOUCH_SCL;
      cfg.freq           = 400000;
      cfg.i2c_addr       = TOUCH_I2C_ADDR;
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
  }
};

#endif // CAP_DISPLAY_BUS_SPI && CAP_TOUCH_GT911
