// ota.cpp — see ota.h.
#if !defined(PANPILOT_SIM)
#include "ota.h"

#include <Arduino.h>
#include <Preferences.h>
#include "esp_ota_ops.h"

namespace hal {
namespace {
Preferences s_p;
bool s_stable_done = false;

void openNs() { static bool o = false; if (!o) { s_p.begin("ota", false); o = true; } }
}  // namespace

void ota_boot_guard() {
  openNs();
  uint32_t cnt = s_p.getUInt("bootcnt", 0) + 1;
  s_p.putUInt("bootcnt", cnt);
  Serial.printf("[ota] boot attempt %u\n", cnt);
  if (cnt >= 3) {
    Serial.println("[ota] boot-loop detected — rolling back");
    s_p.putUInt("bootcnt", 0);
    esp_ota_mark_app_invalid_rollback_and_reboot();   // reverts + reboots
  }
}

void ota_mark_stable() {
  if (s_stable_done) return;
  s_stable_done = true;
  openNs();
  s_p.putUInt("bootcnt", 0);
  const esp_partition_t* run = esp_ota_get_running_partition();
  esp_ota_img_states_t st;
  if (esp_ota_get_state_partition(run, &st) == ESP_OK &&
      st == ESP_OTA_IMG_PENDING_VERIFY) {
    esp_ota_mark_app_valid_cancel_rollback();
    Serial.println("[ota] new image marked valid");
  }
}

}  // namespace hal
#endif
