// timezones.h — a small curated timezone table for the on-device clock setting.
// Each entry carries a POSIX TZ string (with DST rules) so the ESP32's SNTP
// clock handles daylight saving automatically — no twice-a-year fiddling.
// Hardware-free; the UI cycles through it and the HAL feeds the POSIX string to
// configTzTime(). Order is stable (persisted by index).
#pragma once
#include <stdint.h>

struct TimeZone {
  const char* name;    // shown in Settings
  const char* posix;   // POSIX TZ string for configTzTime()
};

static const TimeZone TIMEZONES[] = {
  {"UTC",              "UTC0"},
  {"US Pacific",       "PST8PDT,M3.2.0,M11.1.0"},
  {"US Mountain",      "MST7MDT,M3.2.0,M11.1.0"},
  {"US Arizona",       "MST7"},
  {"US Central",       "CST6CDT,M3.2.0,M11.1.0"},
  {"US Eastern",       "EST5EDT,M3.2.0,M11.1.0"},
  {"UK / Ireland",     "GMT0BST,M3.5.0/1,M10.5.0"},
  {"Central Europe",   "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"India",            "IST-5:30"},
  {"China",            "CST-8"},
  {"Japan",            "JST-9"},
  {"Sydney",           "AEST-10AEDT,M10.1.0,M4.1.0/3"},
};

inline int tz_count() {
  return (int)(sizeof(TIMEZONES) / sizeof(TIMEZONES[0]));
}
inline int tz_clamp(int i) {
  const int n = tz_count();
  if (i < 0) return 0;
  if (i >= n) return n - 1;
  return i;
}
inline const char* tz_name(int i) { return TIMEZONES[tz_clamp(i)].name; }
inline const char* tz_posix(int i) { return TIMEZONES[tz_clamp(i)].posix; }
inline int tz_cycle(int i) { return (tz_clamp(i) + 1) % tz_count(); }
