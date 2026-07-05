// buzzer.h — non-blocking piezo buzzer HAL (base spec §2.3).
// One module routes all alert sounds; mute flag persists to NVS (added M2+).
// Patterns are stepped from loop() via update() so tasks never block (base §13).
#pragma once
#include <stdint.h>

namespace hal {

enum class BuzzPattern : uint8_t {
  None,
  Chirp,    // single short — ack / wake (base spec §2.3)
  Double,   // double chirp — turn-down-now / L2 cue (§3.5)
  Long,     // long — target reached
  Alarm,    // repeating urgent — overheat / L3 (repeats until cleared)
};

void buzzer_begin();
void buzzer_play(BuzzPattern p);
void buzzer_update();          // call every loop() iteration
void buzzer_set_muted(bool m); // mute suppresses all but Alarm (§3.5)
bool buzzer_is_muted();

}  // namespace hal
