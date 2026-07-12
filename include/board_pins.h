// =============================================================================
// board_pins.h — PanPilot single source of truth for every hardware GPIO.
//
// Rule (base spec §2.1): ALL pins live here. Nothing else in the codebase
// hardcodes a GPIO. Each board is a `#if defined(BOARD_*)` block exposing the
// same PIN_* / CAP_* names; shared code is variant-agnostic and reads these.
// Adding a board = a new block + a new [env:*], never a fork. (Pattern adopted
// from the user's BladeKey-Overhead/src/hal/Board.h.)
//
// -----------------------------------------------------------------------------
// PIN PROVENANCE — verified 2026-07-05 (base spec §2.1 "task zero").
//
// PRIMARY BENCH TARGET — CrowPanel Advance 3.5" HMI (ESP32-S3-WROOM-1-N16R8):
//   Wiki:    https://www.elecrow.com/wiki/CrowPanel_Advance_3.5-HMI_ESP32_AI_Display.html
//   Factory: github.com/Elecrow-RD/CrowPanel-Advance-3.5-HMI-ESP32-S3-AI-Powered-IPS-Touch-Screen-480x320
//            example/V1.2_and_V1.3_and_V1.4/Arduino_Code/lesson-03/3_5LVGL/
//            {LovyanGFX_Driver.h,touch.h,3_5LVGL.ino} — the HW V1.2+ example set
//   BENCH-VERIFIED 2026-07-11 on a physical HW V1.2 board (display colors,
//   rotation, GT911 touch). Display pins (SCLK42/MOSI39/DC41/CS40/RST2),
//   backlight GPIO38, GT911 (INT47/RST48/addr 0x14) are identical between the
//   V1.0 and V1.2 factory examples; V1.2 additionally bakes landscape in as
//   offset_rotation=3 (TFT_OFFSET_ROTATION below) and pairs LVGL
//   LV_COLOR_16_SWAP=0 with an lgfx::rgb565_t flush.
//
// CrowPanel Advance 5" siblings (RGB-parallel ST7262, GT911, ESP32-S3):
//   v1.1 — user repo ../cyd-radio/src/config.h  (TCA9534 expander, I2C 15/16)
//   v1.2 — user repo ../BladeKey-Overhead/src/hal/Board.h (BOARD_CROWPANEL_S3_5HMI)
//   Pin maps are CAPTURED below for future targets. Their display bus is RGB
//   parallel (CAP_DISPLAY_BUS_RGB), a path not yet wired — see the CAP flag.
//
// BENCH-CONFIRM ITEMS (see HARDWARE_TEST.md — do not treat as final until James
// confirms on the scope/board): TFT_BL polarity, whether TOUCH_RST is the direct
// GPIO48 or the PCA9557 TP_RST line, BUZZER_PIN (GPIO8 per wiki), battery ADC pin.
// =============================================================================
#pragma once

// ----------------------------------------------------------------------------
// Capability flags (every board defines all of them; 0/1 unless noted)
//   CAP_DISPLAY_BUS_SPI / CAP_DISPLAY_BUS_RGB  — mutually exclusive
//   CAP_TOUCH_GT911 / CAP_TOUCH_XPT2046        — touch controller family
//   CAP_HAS_PSRAM, CAP_HAS_IO_EXPANDER
//   CAP_I2C_SHARED_TOUCH_SENSOR — MLX90640 shares the touch I2C bus (needs a
//     cross-core bus mutex, base spec §4). 0 if the sensor gets its own bus.
// ----------------------------------------------------------------------------

// =====================================================================
#if defined(BOARD_CROWPANEL35_ADVANCE)   // ***** PRIMARY BENCH BOARD *****
// CrowPanel Advance 3.5" HMI — ESP32-S3-WROOM-1-N16R8, ILI9488 480x320 SPI IPS,
// GT911 capacitive touch, PCA9557 I/O expander @ 0x18, onboard buzzer.
  #define BOARD_NAME              "CrowPanel Advance 3.5\" (ESP32-S3)"

  // Display — ILI9488 on SPI2_HOST (LovyanGFX factory config)
  // Write clock: factory ships 40 MHz. ILI9488-over-SPI is 3 bytes/px, so
  // full-screen scrolls are bus-bound (~11 fps at 40 MHz). 80 MHz was BENCH-
  // TESTED 2026-07-11 on HW V1.2 and FAILED — totally scrambled image — so
  // 40 MHz is this panel's ceiling. Do not raise without re-testing on glass.
  #define TFT_SPI_WRITE_HZ        40000000
  #define TFT_SCLK                42
  #define TFT_MOSI                39
  #define TFT_MISO                -1    // not wired (write-only bus)
  #define TFT_DC                  41
  #define TFT_CS                  40
  #define TFT_RST                 2
  #define TFT_BL                  38    // backlight; factory drives HIGH=on.
                                        // PWM (LEDC) for dimming + §3.5 strobe.
  #define TFT_PANEL_WIDTH         320   // native portrait
  #define TFT_PANEL_HEIGHT        480
  // HW V1.2+ factory config (example/V1.2_and_V1.3_and_V1.4/.../lesson-03):
  // offset_rotation = 3 with no setRotation call — i.e. the panel's landscape
  // base orientation is baked into the offset, not the runtime rotation.
  #define TFT_OFFSET_ROTATION     3
  #define TFT_ROTATION            0     // net = offset 3 = landscape 480x320
  #define TFT_INVERT              1     // factory cfg.invert = true
  #define TFT_BUS_SHARED          1     // shares SPI with SD card

  // Touch — GT911 capacitive over I2C (LovyanGFX factory: int47/rst48/0x14)
  #define TOUCH_SDA               15
  #define TOUCH_SCL               16
  #define TOUCH_INT               47
  #define TOUCH_RST               48    // NOTE: PCA9557 also exposes a TP_RST
                                        // line; confirm which is populated.
  #define TOUCH_I2C_ADDR          0x14  // GT911: 0x14 or 0x5D
  #define TOUCH_MAP_X1            480    // factory inverted axis mapping
  #define TOUCH_MAP_X2            0
  #define TOUCH_MAP_Y1            320
  #define TOUCH_MAP_Y2            0

  // Peripheral / sensor I2C — MLX90640 (0x33) shares the touch bus (15/16)
  #define I2C_SDA                 15
  #define I2C_SCL                 16
  #define MLX90640_I2C_ADDR       0x33

  // PCA9557 I/O expander @ 0x18: bit map (MSB..LSB) NC,NC,NC,SHUT,MUTE,TP_RST,
  // LED_BK,NC ; config reg 0x03 = 0xE1. Touch-reset & backlight-enable may route
  // through here in addition to / instead of the direct GPIOs above.
  #define IO_EXPANDER_I2C_ADDR    0x18
  #define EXP_BIT_MUTE            3
  #define EXP_BIT_TP_RST         2
  #define EXP_BIT_LED_BK         1

  // Buzzer — onboard, GPIO8 (wiki). Driven via LEDC PWM by hal/buzzer.
  #define BUZZER_PIN              8

  // Battery ADC — not documented by Elecrow; set once measured. -1 = none yet.
  #define BATTERY_ADC_PIN         -1
  #define BATTERY_USB_DETECT_PIN  -1    // GPIO high when USB present; -1 = n/a

  #define CAP_DISPLAY_BUS_SPI     1
  #define CAP_DISPLAY_BUS_RGB     0
  #define CAP_TOUCH_GT911         1
  #define CAP_TOUCH_XPT2046       0
  #define CAP_HAS_PSRAM           1
  #define CAP_HAS_IO_EXPANDER     1
  #define CAP_I2C_SHARED_TOUCH_SENSOR 1

// =====================================================================
#elif defined(BOARD_CROWPANEL35_BASIC)
// CrowPanel 3.5" basic (DIS05035H) — ESP32-WROVER-B, ILI9488 SPI, XPT2046.
// PINS ARE THE BASE-SPEC §2.1 PLACEHOLDERS — UNVERIFIED. Pull the wiki map for
// the exact basic-board revision before flashing this env.
  #define BOARD_NAME              "CrowPanel 3.5\" basic (ESP32-WROVER) [UNVERIFIED]"
  #define TFT_MISO                12
  #define TFT_MOSI                13
  #define TFT_SCLK                14
  #define TFT_CS                  15
  #define TFT_DC                  2
  #define TFT_RST                 -1
  #define TFT_BL                  27
  #define TFT_PANEL_WIDTH         320
  #define TFT_PANEL_HEIGHT        480
  #define TFT_ROTATION            1
  #define TFT_INVERT              0
  #define TFT_BUS_SHARED          1
  #define TOUCH_CS                33
  #define TOUCH_INT               36    // input-only
  #define TOUCH_IRQ               36    // base-spec alias
  #define I2C_SDA                 21    // exposed header — verify
  #define I2C_SCL                 22
  #define MLX90640_I2C_ADDR       0x33
  #define BUZZER_PIN              -1    // some basic panels have no buzzer
  #define BATTERY_ADC_PIN         -1
  #define CAP_DISPLAY_BUS_SPI     1
  #define CAP_DISPLAY_BUS_RGB     0
  #define CAP_TOUCH_GT911         0
  #define CAP_TOUCH_XPT2046       1
  #define CAP_HAS_PSRAM           1
  #define CAP_HAS_IO_EXPANDER     0
  #define CAP_I2C_SHARED_TOUCH_SENSOR 0   // dedicated I2C header for the sensor

// =====================================================================
#elif defined(BOARD_CROWPANEL_ADV_5_V11)
// CrowPanel Advance 5.0" HMI v1.1 — ESP32-S3, ST7262 800x480 RGB parallel,
// GT911, TCA9534 expander. Pins from ../cyd-radio/src/config.h (captured
// 2026-07-05). Display bus is RGB parallel — HAL path NOT YET WIRED.
  #define BOARD_NAME              "CrowPanel Advance 5\" v1.1 (ESP32-S3, RGB)"
  #define TFT_PANEL_WIDTH         800
  #define TFT_PANEL_HEIGHT        480
  #define RGB_PIN_PCLK            39
  #define RGB_PIN_HSYNC           40
  #define RGB_PIN_VSYNC           41
  #define RGB_PIN_DE              42
  #define RGB_PIN_R0              7
  #define RGB_PIN_R1              17
  #define RGB_PIN_R2              18
  #define RGB_PIN_R3              3
  #define RGB_PIN_R4              46
  #define RGB_PIN_G0              9
  #define RGB_PIN_G1              10
  #define RGB_PIN_G2              11
  #define RGB_PIN_G3              12
  #define RGB_PIN_G4              13
  #define RGB_PIN_G5              14
  #define RGB_PIN_B0              21
  #define RGB_PIN_B1              47
  #define RGB_PIN_B2              48
  #define RGB_PIN_B3              45
  #define RGB_PIN_B4              38
  // ST7262 RGB timings — typical 5" 800x480 (cyd-radio/config.h). PCLK 12 MHz is
  // the empirically-lowest stable rate on this panel. Sync polarity + pclk edge
  // are BENCH-GATED (flip on glass if the panel stays black — see HARDWARE_TEST).
  #define RGB_HSYNC_FRONT_PORCH   8
  #define RGB_HSYNC_PULSE_WIDTH   4
  #define RGB_HSYNC_BACK_PORCH    8
  #define RGB_VSYNC_FRONT_PORCH   8
  #define RGB_VSYNC_PULSE_WIDTH   4
  #define RGB_VSYNC_BACK_PORCH    8
  #define RGB_PCLK_HZ             12000000
  #define RGB_PCLK_IDLE_HIGH      0
  #define TOUCH_SDA               15
  #define TOUCH_SCL               16
  #define TOUCH_INT               -1
  #define TOUCH_RST               -1     // pulsed via the I/O expander, not a GPIO
  #define TOUCH_I2C_ADDR          0x14
  #define I2C_SDA                 15
  #define I2C_SCL                 16
  #define MLX90640_I2C_ADDR       0x33
  // I/O expander drives LCD/touch reset + backlight. v1.1 uses a TCA9534-style
  // bit register at 0x20 (bits below). BENCH-VERIFY the address + bit map before
  // relying on it; some CrowPanel 5" revisions use an STC8 µC (single-byte
  // commands @ 0x30) instead — see ../BladeKey-Overhead/src/hal/Board.h.
  #define IO_EXPANDER_I2C_ADDR    0x20   // TCA9534 (BENCH-VERIFY)
  #define EXP_BIT_TOUCH_RST       1      // EXP_PIN_TOUCH_RST
  #define EXP_BIT_LCD_RST         2      // EXP_PIN_LCD_RST
  #define EXP_BIT_BACKLIGHT       3      // EXP_PIN_BACKLIGHT
  #define EXP_BIT_AMP_SD          4
  #define TFT_BL                  -1     // backlight is behind the expander
  #define BUZZER_PIN              -1
  #define BATTERY_ADC_PIN         -1
  #define CAP_DISPLAY_BUS_SPI     0
  #define CAP_DISPLAY_BUS_RGB     1
  #define CAP_TOUCH_GT911         1
  #define CAP_TOUCH_XPT2046       0
  #define CAP_HAS_PSRAM           1
  #define CAP_HAS_IO_EXPANDER     1
  #define CAP_I2C_SHARED_TOUCH_SENSOR 1

// =====================================================================
#elif defined(BOARD_CROWPANEL_ADV_5_V12)
// CrowPanel Advance 5.0" HMI v1.2 — ESP32-S3, ST7262 800x480 RGB parallel,
// GT911. Pins from ../BladeKey-Overhead/src/hal/Board.h (BOARD_CROWPANEL_S3_5HMI,
// captured 2026-07-05). v1.2 ST7262 needs 21 MHz pclk + pclk_idle_high=1.
// Display bus is RGB parallel — HAL path NOT YET WIRED.
  #define BOARD_NAME              "CrowPanel Advance 5\" v1.2 (ESP32-S3, RGB)"
  #define TFT_PANEL_WIDTH         800
  #define TFT_PANEL_HEIGHT        480
  #define RGB_PIN_PCLK            39
  #define RGB_PIN_HSYNC           40
  #define RGB_PIN_VSYNC           41
  #define RGB_PIN_DE              42
  #define RGB_PIN_R0              7
  #define RGB_PIN_R1              17
  #define RGB_PIN_R2              18
  #define RGB_PIN_R3              3
  #define RGB_PIN_R4              46
  #define RGB_PIN_G0              9
  #define RGB_PIN_G1              10
  #define RGB_PIN_G2              11
  #define RGB_PIN_G3              12
  #define RGB_PIN_G4              13
  #define RGB_PIN_G5              14
  #define RGB_PIN_B0              21
  #define RGB_PIN_B1              47
  #define RGB_PIN_B2              48
  #define RGB_PIN_B3              45
  #define RGB_PIN_B4              38
  // v1.2 factory LovyanGFX driver: 21 MHz pclk + pclk_idle_high=1 (the v1.2
  // ST7262 needs it). Porches match the v1.1 panel. Polarity BENCH-GATED.
  #define RGB_HSYNC_FRONT_PORCH   8
  #define RGB_HSYNC_PULSE_WIDTH   4
  #define RGB_HSYNC_BACK_PORCH    8
  #define RGB_VSYNC_FRONT_PORCH   8
  #define RGB_VSYNC_PULSE_WIDTH   4
  #define RGB_VSYNC_BACK_PORCH    8
  #define RGB_PCLK_HZ             21000000  // v1.2-specific
  #define RGB_PCLK_IDLE_HIGH      1
  #define TOUCH_SDA               15
  #define TOUCH_SCL               16
  #define TOUCH_INT               -1
  #define TOUCH_RST               -1
  #define TOUCH_I2C_ADDR          0x14
  #define I2C_SDA                 15
  #define I2C_SCL                 16
  #define MLX90640_I2C_ADDR       0x33
  // v1.2 backlight is a DIRECT GPIO (IO2), not the expander. The expander story
  // varies by sub-revision (TCA9534 @ 0x18 vs an STC8 µC @ 0x30 with single-byte
  // commands) — BENCH-VERIFY against ../BladeKey-Overhead/src/hal/Board.h before
  // trusting reset/backlight bytes.
  #define IO_EXPANDER_I2C_ADDR    0x18   // (BENCH-VERIFY: may be STC8 @ 0x30)
  #define TFT_BL                  2      // direct-GPIO backlight (active HIGH)
  #define BUZZER_PIN              -1
  #define BATTERY_ADC_PIN         -1
  #define CAP_DISPLAY_BUS_SPI     0
  #define CAP_DISPLAY_BUS_RGB     1
  #define CAP_TOUCH_GT911         1
  #define CAP_TOUCH_XPT2046       0
  #define CAP_HAS_PSRAM           1
  #define CAP_HAS_IO_EXPANDER     1
  #define CAP_I2C_SHARED_TOUCH_SENSOR 1

// =====================================================================
#elif defined(BOARD_SIM)
// Host simulator (SDL2 / LVGL) — no real GPIOs. Present so shared code that
// includes this header compiles natively. See sim/.
  #define BOARD_NAME              "PanPilot Simulator (SDL2)"
  #define TFT_PANEL_WIDTH         320
  #define TFT_PANEL_HEIGHT        480
  #define TFT_ROTATION            1
  #define CAP_DISPLAY_BUS_SPI     0
  #define CAP_DISPLAY_BUS_RGB     0
  #define CAP_TOUCH_GT911         0
  #define CAP_TOUCH_XPT2046       0
  #define CAP_HAS_PSRAM           0
  #define CAP_HAS_IO_EXPANDER     0
  #define CAP_I2C_SHARED_TOUCH_SENSOR 0

#else
  #error "No board selected. Define one of BOARD_CROWPANEL35_ADVANCE (default), \
BOARD_CROWPANEL35_BASIC, BOARD_CROWPANEL_ADV_5_V11, BOARD_CROWPANEL_ADV_5_V12, \
or BOARD_SIM in the PlatformIO env build_flags."
#endif
