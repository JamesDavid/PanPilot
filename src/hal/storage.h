// storage.h — NVS/Preferences persistence (base spec §10). Device-only.
// M2: display unit + mute. Grows: emissivity, custom target, last preset,
// profiles, sessions in later milestones (all version-tagged).
#pragma once
#if !defined(PANPILOT_SIM)

namespace hal {
void storage_begin();
bool storage_get_unit_useF(bool def = true);
void storage_set_unit_useF(bool useF);
bool storage_get_muted(bool def = false);
void storage_set_muted(bool m);
// Target band + last preset (base spec §7.3, §10).
void storage_get_target(int& loF, int& hiF, int& warnF, int& presetId);
void storage_set_target(int loF, int hiF, int warnF, int presetId);
}  // namespace hal

#endif
