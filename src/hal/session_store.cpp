// session_store.cpp — see session_store.h.
#if !defined(PANPILOT_SIM)
#include "session_store.h"

#include <LittleFS.h>
#include <Preferences.h>
#include "app_config.h"

namespace hal {
namespace {
bool s_ok = false;
constexpr int kMaxSessions = 20;   // ~2 MB budget, oldest-first eviction
Preferences s_meta;

// File: /ss/<id>.bin = [SessionSummary][uint16 n][n × int16 tempC10]
String path(uint32_t id) { return "/ss/" + String(id) + ".bin"; }

uint32_t nextId() {
  s_meta.begin("ssmeta", false);
  uint32_t id = s_meta.getUInt("next", 1);
  s_meta.putUInt("next", id + 1);
  s_meta.end();
  return id;
}

void evictIfNeeded() {
  // Count files; if over the cap, delete the lowest-id (oldest) ones.
  File dir = LittleFS.open("/ss");
  if (!dir || !dir.isDirectory()) return;
  uint32_t ids[64]; int n = 0;
  for (File f = dir.openNextFile(); f && n < 64; f = dir.openNextFile()) {
    String nm = f.name(); int dot = nm.indexOf('.');
    if (dot > 0) ids[n++] = nm.substring(0, dot).toInt();
  }
  while (n > kMaxSessions) {
    int lo = 0;
    for (int i = 1; i < n; ++i) if (ids[i] < ids[lo]) lo = i;
    LittleFS.remove(path(ids[lo]));
    ids[lo] = ids[--n];
  }
}
}  // namespace

void sessions_begin() {
  s_ok = LittleFS.begin(true);   // format on first mount if needed
  if (s_ok && !LittleFS.exists("/ss")) LittleFS.mkdir("/ss");
}

void session_store(const SessionSummary& s, const int16_t* trace, uint16_t n) {
  if (!s_ok) return;
  File f = LittleFS.open(path(nextId()), "w");
  if (!f) return;
  f.write((const uint8_t*)&s, sizeof(s));
  f.write((const uint8_t*)&n, sizeof(n));
  if (n && trace) f.write((const uint8_t*)trace, (size_t)n * sizeof(int16_t));
  f.close();
  evictIfNeeded();
}

int sessions_list(uint32_t* ids, int maxIds) {
  if (!s_ok) return 0;
  File dir = LittleFS.open("/ss");
  if (!dir || !dir.isDirectory()) return 0;
  int n = 0;
  for (File f = dir.openNextFile(); f && n < maxIds; f = dir.openNextFile()) {
    String nm = f.name(); int dot = nm.indexOf('.');
    if (dot > 0) ids[n++] = nm.substring(0, dot).toInt();
  }
  for (int i = 0; i < n; ++i)  // sort newest-first (descending id)
    for (int j = i + 1; j < n; ++j)
      if (ids[j] > ids[i]) { uint32_t t = ids[i]; ids[i] = ids[j]; ids[j] = t; }
  return n;
}

bool session_summary(uint32_t id, SessionSummary& out) {
  if (!s_ok) return false;
  File f = LittleFS.open(path(id), "r");
  if (!f) return false;
  bool ok = f.read((uint8_t*)&out, sizeof(out)) == sizeof(out);
  f.close();
  return ok;
}

int session_trace(uint32_t id, int16_t* out, int maxN) {
  if (!s_ok) return 0;
  File f = LittleFS.open(path(id), "r");
  if (!f) return 0;
  f.seek(sizeof(SessionSummary));
  uint16_t n = 0; f.read((uint8_t*)&n, sizeof(n));
  int take = n < maxN ? n : maxN;
  int got = f.read((uint8_t*)out, (size_t)take * sizeof(int16_t)) / sizeof(int16_t);
  f.close();
  return got;
}

String session_csv(uint32_t id) {
  SessionSummary s;
  if (!session_summary(id, s)) return "";
  static int16_t tr[2048];
  int n = session_trace(id, tr, 2048);
  String out = "second,tempC\n";
  for (int i = 0; i < n; ++i) { out += i; out += ','; out += tr[i] / 10.0f; out += '\n'; }
  return out;
}

}  // namespace hal
#endif
