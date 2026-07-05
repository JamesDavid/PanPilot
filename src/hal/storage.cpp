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
void storage_get_target(int& loF, int& hiF, int& warnF, int& presetId) {
  ensure();
  loF = s_prefs.getInt("tlo", 340);
  hiF = s_prefs.getInt("thi", 360);
  warnF = s_prefs.getInt("twarn", 450);
  presetId = s_prefs.getInt("pidx", 5);   // PRESET_GENERIC
}
void storage_set_target(int loF, int hiF, int warnF, int presetId) {
  ensure();
  s_prefs.putInt("tlo", loF);
  s_prefs.putInt("thi", hiF);
  s_prefs.putInt("twarn", warnF);
  s_prefs.putInt("pidx", presetId);
}

}  // namespace hal
#endif
