// main.cpp — PanPilot entry point (base spec §3: setup + task spawn only).
// M2: Thermometer Mode. SensorTask (core 0) reads the MLX90640, runs
// frame_analysis + thermal_model, and publishes a UiState snapshot; the UI loop
// (core 1) renders the home screen (temp/rate/trend) with the thermal view one
// tap away, and dumps readings over serial.
#if !defined(PANPILOT_SIM)

#include <Arduino.h>
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "app_config.h"
#include "board_pins.h"
#include "pan_types.h"
#include "core/app_state.h"
#include "core/thermal_model.h"
#include "hal/display.h"
#include "hal/buzzer.h"
#include "hal/storage.h"
#include "hal/i2c_bus.h"
#include "sensor/mlx90640_source.h"
#include "sensor/frame_analysis.h"
#include "ui/ui_root.h"

namespace {

struct Snapshot {
  ThermalFrame frame;
  PanReading reading;
  UiState ui;
  bool has = false;
} g_snap;
SemaphoreHandle_t g_snap_mtx = nullptr;
volatile bool g_sensor_ok = false;

void SensorTask(void*) {
  FrameAnalyzer analyzer;
  ThermalModel model;
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
      model.update(r);

      UiState u;
      u.mode = Mode::THERMOMETER;
      u.presence = r.presence;
      u.modelValid = model.valid();
      u.displayTempC = model.displayTempC();
      u.rateCPerMin = model.rateCPerMin();
      u.trend = model.trend();
      u.confidence = r.confidence;
      u.moved = r.moved;
      u.stainlessHint = r.stainlessHint;
      u.muted = hal::buzzer_is_muted();

      if (xSemaphoreTake(g_snap_mtx, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_snap.frame = frame; g_snap.reading = r; g_snap.ui = u; g_snap.has = true;
        xSemaphoreGive(g_snap_mtx);
      }
    }
    vTaskDelay(period);
  }
}

void persist_unit(bool useF) { hal::storage_set_unit_useF(useF); }

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
  hal::storage_begin();
  hal::display_begin();
  hal::buzzer_begin();
  hal::buzzer_set_muted(hal::storage_get_muted());

  ui::root_init(hal::storage_get_unit_useF(), persist_unit);

  g_snap_mtx = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(SensorTask, "sensor", 8192, nullptr, 2, nullptr, 0);

  hal::buzzer_play(hal::BuzzPattern::Chirp);
  Serial.println("[PanPilot] M2 ready — Thermometer Mode");
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
      ui::root_update(s.frame, s.reading, s.ui);
      const PanReading& r = s.reading;
      Serial.printf("[reading] %s pan=%.1fC disp=%.1fC rate=%.1fC/min conf=%u%s\n",
                    presence_str(r.presence), r.panTempC, s.ui.displayTempC,
                    s.ui.rateCPerMin, r.confidence,
                    r.stainlessHint ? " STAINLESS?" : "");
    }
  }
  delay(UI_TICK_MS);
}

#endif  // !PANPILOT_SIM
