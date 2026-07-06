// storage.h — NVS/Preferences persistence (base spec §10). Device-only.
// M2: display unit + mute. Grows: emissivity, custom target, last preset,
// profiles, sessions in later milestones (all version-tagged).
#pragma once
#if !defined(PANPILOT_SIM)
#include <Arduino.h>
#include "core/session.h"
#include "core/profiles.h"

namespace hal {
void storage_begin();
bool storage_get_unit_useF(bool def = true);
void storage_set_unit_useF(bool useF);
bool storage_get_muted(bool def = false);
void storage_set_muted(bool m);
uint8_t storage_get_brightness(uint8_t def = 2);  // Settings backlight level 0/1/2
void storage_set_brightness(uint8_t level);
// Target band + last preset (base spec §7.3, §10).
void storage_get_target(int& loF, int& hiF, int& warnF, int& presetId);
void storage_set_target(int loF, int hiF, int warnF, int presetId);
// Session ring (base spec §8, §10): newest-first, capped at SESSION_RING_SIZE.
void storage_session_push(const SessionSummary& s);
int  storage_session_count();
bool storage_session_get(int newestIndex, SessionSummary& out);
// Active pan profile (Learn Pan Mode, base spec §10).
void storage_save_profile(const PanProfile& p);
bool storage_load_profile(PanProfile& out);
// MQTT broker (M9) — empty string disables MQTT/HA.
String storage_get_mqtt_broker();
void storage_set_mqtt_broker(const String& b);
}  // namespace hal

#endif
