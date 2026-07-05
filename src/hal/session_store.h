// session_store.h — LittleFS-backed session log + 1 Hz trace (roadmap §2.3).
// Device-only. Replaces the M5 NVS ring: one compact binary record per session
// plus an optional temperature trace, newest-first, capped by count (~2 MB
// budget). The web UI browses these and downloads CSV.
#pragma once
#if !defined(PANPILOT_SIM)
#include <Arduino.h>
#include "core/session.h"

namespace hal {
void sessions_begin();     // mount LittleFS
// Store a finished session + its trace (tempC*10, 1 Hz). n may be 0.
void session_store(const SessionSummary& s, const int16_t* traceC10, uint16_t n);
int  sessions_list(uint32_t* idsOut, int maxIds);       // newest-first, returns count
bool session_summary(uint32_t id, SessionSummary& out);
int  session_trace(uint32_t id, int16_t* out, int maxN);
String session_csv(uint32_t id);
}  // namespace hal
#endif
