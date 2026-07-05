// main.cpp — PanPilot entry point (base spec §3: setup + task spawn only).
// M0: single-loop bring-up (display + touch + backlight + buzzer + LVGL UI).
// The SensorTask/LogicTask/UITask split (base spec §4) arrives with M1 when the
// thermal pipeline exists; at M0 there is no sensor to service.
#if !defined(PANPILOT_SIM)

#include <Arduino.h>
#include <lvgl.h>

#include "app_config.h"
#include "board_pins.h"
#include "hal/display.h"
#include "hal/buzzer.h"
#include "ui/ui_root.h"

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.printf("\n[PanPilot] %s  fw=%s\n", BOARD_NAME, PANPILOT_FW_VERSION);

  hal::display_begin();   // LovyanGFX + LVGL display/touch + backlight
  hal::buzzer_begin();
  ui::root_init();

  hal::buzzer_play(hal::BuzzPattern::Chirp);  // boot chirp — audible sign of life
  Serial.println("[PanPilot] M0 ready — tap BEEP");
}

void loop() {
  lv_timer_handler();     // LVGL render + touch
  hal::buzzer_update();   // step any active buzzer pattern (non-blocking)
  delay(UI_TICK_MS);
}

#endif  // !PANPILOT_SIM
