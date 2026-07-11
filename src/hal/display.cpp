// display.cpp — LovyanGFX <-> LVGL glue. Selects the panel device by capability
// flag: SPI ILI9488 (CrowPanel 3.5") or RGB-parallel ST7262 (CrowPanel 5").
// Device-only (Arduino). The simulator provides its own LVGL display in sim/.
#if !defined(PANPILOT_SIM)

#include <Arduino.h>
#include <lvgl.h>
#include "board_pins.h"     // defines CAP_DISPLAY_BUS_* — must precede the #if below
#include "display.h"
#include "app_config.h"
#include "i2c_bus.h"

#if CAP_DISPLAY_BUS_RGB
  #include "lgfx_panpilot_rgb.h"
  #include <Wire.h>
#else
  #include "lgfx_panpilot.h"
#endif

namespace {

#if CAP_DISPLAY_BUS_RGB
LGFX_PanPilotRGB lcd;
#else
LGFX_PanPilot lcd;
#endif

// The UI is authored at 480x320 (base spec §9). On a panel bigger than that
// (5" RGB is 800x480) the UI is centered; on the 3.5" SPI panels the panel IS
// 480x320 so the offsets are zero.
constexpr uint16_t kHor = 480;
constexpr uint16_t kVer = 320;
#if CAP_DISPLAY_BUS_RGB
constexpr int kOffX = (TFT_PANEL_WIDTH  - (int)kHor) / 2;
constexpr int kOffY = (TFT_PANEL_HEIGHT - (int)kVer) / 2;
#else
constexpr int kOffX = 0;
constexpr int kOffY = 0;
#endif

// Two partial draw buffers in internal RAM. The classic ESP32-WROVER (basic
// board) has much tighter DRAM, so it gets 8-line buffers; the S3 keeps 16.
#ifdef BOARD_CROWPANEL35_BASIC
constexpr uint32_t kBufPx = kHor * 8;
#else
constexpr uint32_t kBufPx = kHor * 16;
#endif
lv_color_t s_buf1[kBufPx];
lv_color_t s_buf2[kBufPx];
lv_disp_draw_buf_t s_draw_buf;
lv_disp_drv_t s_disp_drv;
lv_indev_drv_t s_indev_drv;

void flush_cb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* px) {
  const uint32_t w = area->x2 - area->x1 + 1;
  const uint32_t h = area->y2 - area->y1 + 1;
  // LVGL renders with LV_COLOR_16_SWAP=1, i.e. the buffer is ALREADY in panel
  // byte order — the lgfx::rgb565_t cast tells LovyanGFX exactly that (this is
  // the factory lesson-03 flush verbatim). The old writePixels(..., swap=true)
  // swapped a second time: bench symptom was a pink background, lime buttons,
  // and fuzzy text (every color's bytes reversed). DMA is safe here because
  // LVGL double-buffers (s_buf1/s_buf2).
  lcd.pushImageDMA(area->x1 + kOffX, area->y1 + kOffY, w, h,
                   reinterpret_cast<lgfx::rgb565_t*>(px));
  lv_disp_flush_ready(drv);
}

void touch_read_cb(lv_indev_drv_t* drv, lv_indev_data_t* data) {
  uint16_t x = 0, y = 0;
  // GT911 shares the I2C bus with the MLX90640 (base spec §4) — serialize.
  hal::I2CGuard guard(10);
  if (guard.held && lcd.getTouch(&x, &y)) {
    const int lx = (int)x - kOffX, ly = (int)y - kOffY;   // panel -> UI coords
    if (lx >= 0 && lx < (int)kHor && ly >= 0 && ly < (int)kVer) {
      data->state = LV_INDEV_STATE_PRESSED;
      data->point.x = lx;
      data->point.y = ly;
      return;
    }
  }
  data->state = LV_INDEV_STATE_RELEASED;
}

#if CAP_DISPLAY_BUS_RGB
// I/O-expander bring-up for the RGB panels (release LCD/touch reset, backlight
// on). BENCH-GATED: assumes a TCA9534-style config(0x03)/output(0x01) register
// map; some 5" revisions use an STC8 uC @ 0x30 with single-byte commands
// instead (see board_pins.h). Verify on glass before trusting these bytes.
void panel_power_on() {
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
#if defined(IO_EXPANDER_I2C_ADDR) && defined(EXP_BIT_LCD_RST)
  auto wr = [](uint8_t reg, uint8_t val) {
    Wire.beginTransmission(IO_EXPANDER_I2C_ADDR);
    Wire.write(reg); Wire.write(val); Wire.endTransmission();
  };
  const uint8_t outMask = (uint8_t)((1u << EXP_BIT_LCD_RST) |
                                    (1u << EXP_BIT_TOUCH_RST) |
                                    (1u << EXP_BIT_BACKLIGHT));
  wr(0x03, (uint8_t)~outMask);   // config: 0 = output for our bits
  wr(0x01, 0x00);                // assert resets low
  delay(20);
  wr(0x01, outMask);             // release resets + backlight on
  delay(50);
#endif
#if defined(TFT_BL) && (TFT_BL >= 0)
  pinMode(TFT_BL, OUTPUT);       // direct-GPIO backlight (v1.2)
  digitalWrite(TFT_BL, HIGH);
#endif
}
#endif

}  // namespace

// LVGL custom-tick source (lv_conf.h -> panpilot_millis()).
extern "C" uint32_t panpilot_millis(void) { return millis(); }

namespace hal {

void display_begin() {
#if CAP_DISPLAY_BUS_RGB
  panel_power_on();              // expander/backlight before the panel inits
  lcd.init();
  lcd.setRotation(0);            // ST7262 is native 800x480 landscape
#else
  lcd.init();
  lcd.setRotation(TFT_ROTATION);
  lcd.setBrightness(BACKLIGHT_ACTIVE_BRIGHT);
#endif

  lv_init();
  lv_disp_draw_buf_init(&s_draw_buf, s_buf1, s_buf2, kBufPx);

  lv_disp_drv_init(&s_disp_drv);
  s_disp_drv.hor_res = kHor;
  s_disp_drv.ver_res = kVer;
  s_disp_drv.flush_cb = flush_cb;
  s_disp_drv.draw_buf = &s_draw_buf;
  lv_disp_drv_register(&s_disp_drv);

  lv_indev_drv_init(&s_indev_drv);
  s_indev_drv.type = LV_INDEV_TYPE_POINTER;
  s_indev_drv.read_cb = touch_read_cb;
  lv_indev_drv_register(&s_indev_drv);
}

void display_set_brightness(uint8_t level) { lcd.setBrightness(level); }

}  // namespace hal

#endif  // !PANPILOT_SIM
