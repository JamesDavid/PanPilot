// weblog.h — serial tee + RAM log ring for Wi-Fi debugging (bench 2026-07-12:
// "can you do everything over wifi if serial isn't available?"). Every
// Serial.* call in the firmware (and in libraries that print through Serial)
// is transparently redirected to PanPilotTee via serial_tee_redirect.h, which
// writes to the REAL UART as always AND into a ring buffer served at /log —
// so a device mounted at the stove can be debugged without the cable.
//
// Device-only. The ring is written from both cores (SensorTask logs too), so
// pushes take a spinlock; snapshots copy under the same lock.
#pragma once
#if !defined(PANPILOT_SIM)
#include <Arduino.h>

class TeeLog : public Stream {
 public:
  void begin(unsigned long baud);
  size_t write(uint8_t b) override;
  size_t write(const uint8_t* buf, size_t n) override;
  int available() override;
  int read() override;
  int peek() override;
  void flush() override;
};

extern TeeLog PanPilotTee;

// Copy the ring's contents (oldest first) into `out`, returning bytes copied.
// `cap` must include room for the NUL terminator.
size_t weblog_snapshot(char* out, size_t cap);

#endif  // !PANPILOT_SIM
