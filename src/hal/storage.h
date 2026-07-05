// storage.h — NVS/Preferences persistence (base spec §10). Device-only.
// M2: display unit + mute. Grows: emissivity, custom target, last preset,
// profiles, sessions in later milestones (all version-tagged).
#pragma once
#if !defined(PANPILOT_SIM)
#include "core/session.h"

namespace hal {
void storage_begin();
bool storage_get_unit_useF(bool def = true);
void storage_set_unit_useF(bool useF);
bool storage_get_muted(bool def = false);
void storage_set_muted(bool m);
// Target band + last preset (base spec §7.3, §10).
void storage_get_target(int& loF, int& hiF, int& warnF, int& presetId);
void storage_set_target(int loF, int hiF, int warnF, int presetId);
// Session ring (base spec §8, §10): newest-first, capped at SESSION_RING_SIZE.
void storage_session_push(const SessionSummary& s);
int  storage_session_count();
bool storage_session_get(int newestIndex, SessionSummary& out);
}  // namespace hal

#endif
