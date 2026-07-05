// storage.cpp — see storage.h. ESP32 Preferences (NVS namespace "cfg").
#if !defined(PANPILOT_SIM)
#include "storage.h"
#include <Preferences.h>

namespace hal {
namespace {
Preferences s_prefs;
constexpr const char* NS = "cfg";
bool s_open = false;
void ensure() { if (!s_open) { s_prefs.begin(NS, false); s_open = true; } }
}  // namespace

void storage_begin() { ensure(); }

bool storage_get_unit_useF(bool def) { ensure(); return s_prefs.getBool("useF", def); }
void storage_set_unit_useF(bool useF) { ensure(); s_prefs.putBool("useF", useF); }
bool storage_get_muted(bool def) { ensure(); return s_prefs.getBool("mute", def); }
void storage_set_muted(bool m) { ensure(); s_prefs.putBool("mute", m); }

}  // namespace hal
#endif
