// ha.h — MQTT + Home Assistant auto-discovery bridge (roadmap spec §2.2).
// Device-only, ENABLE_WIFI. Publishes retained discovery configs so entities
// appear in HA automatically, streams state, and accepts commands (mute, target,
// preset) back. This is the bridge Phase 3 smart-plug control rides on, so it is
// kept clean. Compile-verified; broker behavior is bench-tested (HARDWARE_TEST M9).
#pragma once
#if !defined(PANPILOT_SIM) && defined(ENABLE_WIFI)
#include "core/app_state.h"

namespace ha {
using MuteCb = void (*)(bool);
using TargetCb = void (*)(int centerF);
using PresetCb = void (*)(uint8_t presetId);

void begin(const char* broker, uint16_t port, MuteCb, TargetCb, PresetCb);
void loop();                        // reconnect + pump MQTT
void publish(const UiState& s, bool useF);
bool enabled();
bool connected();                   // MQTT link up (actuator liveness for S7/S8)
void actuator_publish(const char* topic, const char* payload);  // ASSIST duty out
void publishAttention(int level, const char* verb, const char* sub);  // §3.5 mirror
}  // namespace ha
#endif
