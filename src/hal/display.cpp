// display.cpp — LovyanGFX <-> LVGL glue for SPI/GT911 CrowPanel boards.
// Device-only (Arduino). The simulator provides its own LVGL display in sim/.
#if !defined(PANPILOT_SIM)

#include <Arduino.h>
#include <lvgl.h>
#include "display.h"
#include "lgfx_panpilot.h"
#include "app_config.h"

namespace {

LGFX_PanPilot lcd;

// Landscape 480x320 (base spec §9). Native panel is 320x480 portrait; rotation 1.
constexpr uint16_t kHor = 480;
constexpr uint16_t kVer = 320;

// Two partial draw buffers, 16 lines each, in DMA-capable internal RAM. 16 (not
// 32) lines keeps .bss within the classic ESP32-WROVER's tighter DRAM while
// staying smooth on the S3 (base spec §4). ~30 KB total.
constexpr uint32_t kBufPx = kHor * 16;
lv_color_t s_buf1[kBufPx];
lv_color_t s_buf2[kBufPx];
lv_disp_draw_buf_t s_draw_buf;
lv_disp_drv_t s_disp_drv;
lv_indev_drv_t s_indev_drv;

void flush_cb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* px) {
  const uint32_t w = area->x2 - area->x1 + 1;
  const uint32_t h = area->y2 - area->y1 + 1;
  lcd.startWrite();
  lcd.setAddrWindow(area->x1, area->y1, w, h);
  lcd.writePixels(reinterpret_cast<uint16_t*>(px), w * h, true);
  lcd.endWrite();
  lv_disp_flush_ready(drv);
}

void touch_read_cb(lv_indev_drv_t* drv, lv_indev_data_t* data) {
  uint16_t x = 0, y = 0;
  if (lcd.getTouch(&x, &y)) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

}  // namespace

// LVGL custom-tick source (lv_conf.h -> panpilot_millis()).
extern "C" uint32_t panpilot_millis(void) { return millis(); }

namespace hal {

void display_begin() {
  lcd.init();
  lcd.setRotation(TFT_ROTATION);
  lcd.setBrightness(BACKLIGHT_ACTIVE_BRIGHT);

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
