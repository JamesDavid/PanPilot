// weblog.cpp — see weblog.h. This TU is the ONE place that must talk to the
// REAL UART. The -include redirect runs BEFORE this file's first line, so
// defining PANPILOT_TEE_IMPL here is too late — the macro is already active
// and `::Serial` would name the tee itself: write() tail-calls write(), the
// optimizer turns the recursion into a jump, and boot hangs silently at the
// first Serial.begin (bench-found 2026-07-12: ROM banner then nothing).
#ifdef Serial
#undef Serial
#endif
#if !defined(PANPILOT_SIM)
#include "hal/weblog.h"

namespace {
constexpr size_t RING = 6144;   // ~ the last few minutes of typical output
char s_ring[RING];
size_t s_head = 0;              // total bytes ever written (mod for index)
portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

inline void push(uint8_t b) {
  taskENTER_CRITICAL(&s_mux);
  s_ring[s_head % RING] = (char)b;
  ++s_head;
  taskEXIT_CRITICAL(&s_mux);
}
}  // namespace

void TeeLog::begin(unsigned long baud) { ::Serial.begin(baud); }
size_t TeeLog::write(uint8_t b) {
  push(b);
  return ::Serial.write(b);
}
size_t TeeLog::write(const uint8_t* buf, size_t n) {
  for (size_t i = 0; i < n; ++i) push(buf[i]);
  return ::Serial.write(buf, n);
}
int TeeLog::available() { return ::Serial.available(); }
int TeeLog::read() { return ::Serial.read(); }
int TeeLog::peek() { return ::Serial.peek(); }
void TeeLog::flush() { ::Serial.flush(); }

TeeLog PanPilotTee;

size_t weblog_snapshot(char* out, size_t cap) {
  if (!out || cap == 0) return 0;
  taskENTER_CRITICAL(&s_mux);
  const size_t total = s_head;
  const size_t n0 = total < RING ? total : RING;      // bytes available
  const size_t n = n0 < cap - 1 ? n0 : cap - 1;       // bytes we can copy
  const size_t start = (total - n) % RING;            // oldest byte's index
  const size_t first = n < RING - start ? n : RING - start;
  memcpy(out, s_ring + start, first);                 // two memcpys keep the
  if (n > first) memcpy(out + first, s_ring, n - first);  // IRQ-off window ~us
  taskEXIT_CRITICAL(&s_mux);
  out[n] = '\0';
  return n;
}
#endif  // !PANPILOT_SIM
