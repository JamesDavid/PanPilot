// attention.h — attention & cue escalation (roadmap spec §3.5). Hardware-free +
// unit-tested. Every cue (trend tick, READY, TURN DOWN NOW, FLIP, alarms) routes
// through one manager with four escalation levels; it decides when to buzz and
// strobe, honoring mute (never suppresses L3) and per-cue re-arm. The device HAL
// maps the output to buzzer patterns + a backlight strobe.
#pragma once
#include <stdint.h>
#include "app_config.h"

enum class AttnLevel : uint8_t { L0, L1, L2, L3 };  // passive/notify/act/alarm

struct AttnOutput {
  AttnLevel level = AttnLevel::L0;
  bool buzz = false;        // emit the level's buzzer pattern this tick
  bool strobe = false;      // backlight strobe (L2/L3)
  const char* verb = "";    // instruction-card verb (L2+): "TURN DOWN NOW"
  const char* sub = "";
};

class AttentionManager {
 public:
  void setMuted(bool m) { muted_ = m; }

  // Raise a cue. Re-raising the identical (level, verb) is a no-op so a
  // persistent condition doesn't retrigger the entry beep.
  void raise(AttnLevel level, const char* verb, const char* sub, uint32_t now);
  void ack(uint32_t now);     // user acknowledged (dismiss L2/L3)
  void clear();               // condition resolved
  AttnOutput tick(uint32_t now);

 private:
  bool muted_ = false;
  bool active_ = false;
  AttnLevel level_ = AttnLevel::L0;
  const char* verb_ = "";
  const char* sub_ = "";
  uint32_t last_buzz_ = 0;
  bool entered_ = false;      // pending entry beep
};
