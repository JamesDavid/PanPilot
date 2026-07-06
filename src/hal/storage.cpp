// storage.cpp — see storage.h. ESP32 Preferences (NVS namespace "cfg").
#if !defined(PANPILOT_SIM)
#include "storage.h"
#include <Preferences.h>
#include "app_config.h"
#include <cstdio>

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
uint8_t storage_get_brightness(uint8_t def) { ensure(); return (uint8_t)s_prefs.getUChar("bright", def); }
void storage_set_brightness(uint8_t level) { ensure(); s_prefs.putUChar("bright", level); }
bool storage_get_onboarded(bool def) { ensure(); return s_prefs.getBool("onboard", def); }
void storage_set_onboarded(bool done) { ensure(); s_prefs.putBool("onboard", done); }
int storage_get_timezone(int def) { ensure(); return s_prefs.getInt("tz", def); }
void storage_set_timezone(int idx) { ensure(); s_prefs.putInt("tz", idx); }
bool storage_get_gains(float& kp, float& ki, float& kd) {
  ensure();
  float g[3];
  if (s_prefs.getBytes("pidg", g, sizeof(g)) != sizeof(g)) return false;
  kp = g[0]; ki = g[1]; kd = g[2];
  return true;
}
void storage_set_gains(float kp, float ki, float kd) {
  ensure();
  float g[3] = {kp, ki, kd};
  s_prefs.putBytes("pidg", g, sizeof(g));
}
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

// Session ring: a head index + count, entries under keys s0..s(N-1).
void storage_session_push(const SessionSummary& s) {
  ensure();
  int head = s_prefs.getInt("shead", 0);
  int count = s_prefs.getInt("scount", 0);
  char key[8];
  std::snprintf(key, sizeof(key), "s%d", head);
  s_prefs.putBytes(key, &s, sizeof(s));
  head = (head + 1) % SESSION_RING_SIZE;
  if (count < SESSION_RING_SIZE) ++count;
  s_prefs.putInt("shead", head);
  s_prefs.putInt("scount", count);
}

int storage_session_count() { ensure(); return s_prefs.getInt("scount", 0); }

bool storage_session_get(int newestIndex, SessionSummary& out) {
  ensure();
  const int count = s_prefs.getInt("scount", 0);
  if (newestIndex < 0 || newestIndex >= count) return false;
  const int head = s_prefs.getInt("shead", 0);
  // head points at the next write slot; newest is head-1.
  const int slot = ((head - 1 - newestIndex) % SESSION_RING_SIZE
                    + SESSION_RING_SIZE) % SESSION_RING_SIZE;
  char key[8];
  std::snprintf(key, sizeof(key), "s%d", slot);
  return s_prefs.getBytes(key, &out, sizeof(out)) == sizeof(out);
}

void storage_save_profile(const PanProfile& p) {
  ensure();
  s_prefs.putBytes("profile", &p, sizeof(p));
}
bool storage_load_profile(PanProfile& out) {
  ensure();
  if (s_prefs.getBytes("profile", &out, sizeof(out)) != sizeof(out)) return false;
  return out.magic == PANPROFILE_MAGIC && out.version == PANPROFILE_VERSION &&
         out.valid;
}

uint32_t storage_get_profiles(void* out, uint32_t maxBytes) {
  ensure();
  return (uint32_t)s_prefs.getBytes("profs", out, maxBytes);
}
void storage_set_profiles(const void* data, uint32_t bytes) {
  ensure();
  if (bytes == 0) s_prefs.remove("profs");
  else s_prefs.putBytes("profs", data, bytes);
}
int storage_get_active_profile(int def) { ensure(); return s_prefs.getInt("profact", def); }
void storage_set_active_profile(int idx) { ensure(); s_prefs.putInt("profact", idx); }

String storage_get_mqtt_broker() { ensure(); return s_prefs.getString("mqtt", ""); }
void storage_set_mqtt_broker(const String& b) { ensure(); s_prefs.putString("mqtt", b); }

uint32_t storage_get_foodfb(void* out, uint32_t maxBytes) {
  ensure();
  return (uint32_t)s_prefs.getBytes("foodfb", out, maxBytes);
}
void storage_set_foodfb(const void* data, uint32_t bytes) {
  ensure();
  s_prefs.putBytes("foodfb", data, bytes);
}

uint32_t storage_get_custom_presets(void* out, uint32_t maxBytes) {
  ensure();
  return (uint32_t)s_prefs.getBytes("cpresets", out, maxBytes);
}
void storage_set_custom_presets(const void* data, uint32_t bytes) {
  ensure();
  if (bytes == 0) s_prefs.remove("cpresets");
  else s_prefs.putBytes("cpresets", data, bytes);
}

}  // namespace hal
#endif
