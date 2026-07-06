// net.h — Wi-Fi provisioning + embedded web dashboard (roadmap spec §2.2).
// Device-only, compiled when ENABLE_WIFI is defined. Local-first: every cooking
// feature works with Wi-Fi off; this is a convenience mirror. Compile-verified;
// on-device behavior is bench-tested (HARDWARE_TEST M8).
#pragma once
#if !defined(PANPILOT_SIM) && defined(ENABLE_WIFI)
#include <Arduino.h>
#include "core/app_state.h"
#include "pan_types.h"

namespace net {
// Web settings mirror (Phase 2). Callbacks fire on the loop core (net::loop),
// never from the async web task, so they can touch NVS/UI safely.
using UnitCb = void (*)(bool useF);
using SetMuteCb = void (*)(bool muted);
using SetBrightCb = void (*)(uint8_t level);
using SetTzCb = void (*)(uint8_t tzIndex);

void begin();                 // WiFiManager (non-blocking) + server + mDNS
void loop();                  // housekeeping (portal + WS cleanup) + apply web edits
void publishState(const UiState& s, bool useF);
void publishThermal(const ThermalFrame& f);
bool connected();
String mqtt_broker();          // MQTT broker captured at provisioning ("" = off)
void set_settings_cbs(UnitCb, SetMuteCb, SetBrightCb, SetTzCb);
}  // namespace net
#endif
