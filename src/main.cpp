// main.cpp — PanPilot entry point (base spec §3: setup + task spawn only).
// M1: thermal pipeline. SensorTask (core 0) reads the MLX90640, runs
// frame_analysis, and publishes a PanReading + frame snapshot; the UI loop
// (core 1) renders the live thermal view and serial-dumps PanReading (§4).
#if !defined(PANPILOT_SIM)

#include <Arduino.h>
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "app_config.h"
#include "board_pins.h"
#include "pan_types.h"
#include "hal/display.h"
#include "hal/buzzer.h"
#include "hal/i2c_bus.h"
#include "sensor/mlx90640_source.h"
#include "sensor/frame_analysis.h"
#include "ui/screen_thermal.h"

namespace {

// Snapshot shared SensorTask -> UI (double-copy under a short mutex, §4).
struct Snapshot {
  ThermalFrame frame;
  PanReading reading;
  bool has = false;
} g_snap;
SemaphoreHandle_t g_snap_mtx = nullptr;
volatile bool g_sensor_ok = false;

void SensorTask(void*) {
  FrameAnalyzer analyzer;
  ThermalFrame frame;
  const TickType_t period = pdMS_TO_TICKS(1000 / MLX_REFRESH_HZ);
  for (;;) {
    if (!g_sensor_ok) {
      g_sensor_ok = sensor::mlx_begin();
      if (!g_sensor_ok) { vTaskDelay(pdMS_TO_TICKS(2000)); continue; }
      Serial.println("[PanPilot] MLX90640 online");
    }
    if (sensor::mlx_read(frame)) {
      PanReading r = analyzer.process(frame);
      if (xSemaphoreTake(g_snap_mtx, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_snap.frame = frame;
        g_snap.reading = r;
        g_snap.has = true;
        xSemaphoreGive(g_snap_mtx);
      }
    }
    vTaskDelay(period);
  }
}

const char* presence_str(PanPresence p) {
  switch (p) {
    case PanPresence::PRESENT: return "PRESENT";
    case PanPresence::ABSENT: return "ABSENT";
    case PanPresence::OBSTRUCTED: return "OBSTRUCTED";
    default: return "UNCERTAIN";
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.printf("\n[PanPilot] %s  fw=%s\n", BOARD_NAME, PANPILOT_FW_VERSION);

  hal::i2c_bus_init();
  hal::display_begin();
  hal::buzzer_begin();
  ui::thermal_create();

  g_snap_mtx = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(SensorTask, "sensor", 8192, nullptr, 2, nullptr, 0);

  hal::buzzer_play(hal::BuzzPattern::Chirp);
  Serial.println("[PanPilot] M1 ready — live thermal view");
}

void loop() {
  static uint32_t last = 0;
  lv_timer_handler();
  hal::buzzer_update();

  const uint32_t now = millis();
  if (now - last >= 250) {          // 4 Hz UI refresh + serial dump
    last = now;
    Snapshot s;
    if (g_snap_mtx && xSemaphoreTake(g_snap_mtx, pdMS_TO_TICKS(5)) == pdTRUE) {
      s = g_snap;
      xSemaphoreGive(g_snap_mtx);
    }
    if (s.has) {
      ui::thermal_update(s.frame, s.reading, /*useF=*/true);
      const PanReading& r = s.reading;
      Serial.printf("[reading] %s pan=%.1fC (%.0fF) conf=%u roi=%u c=(%.1f,%.1f)%s\n",
                    presence_str(r.presence), r.panTempC,
                    r.panTempC * 9 / 5 + 32, r.confidence, r.roiPixelCount,
                    r.roiCx, r.roiCy, r.stainlessHint ? " STAINLESS?" : "");
    }
  }
  delay(UI_TICK_MS);
}

#endif  // !PANPILOT_SIM
