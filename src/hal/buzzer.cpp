// buzzer.cpp — device (Arduino LEDC) implementation of the buzzer HAL.
#if !defined(PANPILOT_SIM)

#include <Arduino.h>
#include "buzzer.h"
#include "board_pins.h"
#include "app_config.h"

namespace hal {
namespace {

constexpr int kLedcChannel = 0;      // buzzer LEDC ch (backlight uses ch 7)
bool s_muted = false;

// A pattern is a list of (on, duration_ms) segments stepped by millis().
struct Seg { bool on; uint16_t ms; };
const Seg kChirp[]  = {{true, BUZZER_CHIRP_MS}, {false, 0}};
const Seg kDouble[] = {{true, BUZZER_CHIRP_MS}, {false, 70}, {true, BUZZER_CHIRP_MS}, {false, 0}};
const Seg kLong[]   = {{true, 400}, {false, 0}};
const Seg kAlarm[]  = {{true, 200}, {false, 150}, {true, 200}, {false, 150},
                       {true, 200}, {false, 600}, {true, 0}};  // last {,0} => repeat

const Seg*  s_seq = nullptr;
uint8_t     s_idx = 0;
uint32_t    s_segStart = 0;
bool        s_repeat = false;

inline bool active() {
#if BUZZER_PIN < 0
  return false;
#else
  return true;
#endif
}

void toneOn(bool on) {
#if BUZZER_PIN >= 0
  if (on) ledcWriteTone(kLedcChannel, BUZZER_PWM_FREQ_HZ);
  else    ledcWriteTone(kLedcChannel, 0);
#else
  (void)on;
#endif
}

}  // namespace

void buzzer_begin() {
#if BUZZER_PIN >= 0
  ledcSetup(kLedcChannel, BUZZER_PWM_FREQ_HZ, BUZZER_PWM_BITS);
  ledcAttachPin(BUZZER_PIN, kLedcChannel);
  toneOn(false);
#endif
}

void buzzer_play(BuzzPattern p) {
  if (!active()) return;
  if (s_muted && p != BuzzPattern::Alarm) return;
  s_repeat = false;
  switch (p) {
    case BuzzPattern::Chirp:  s_seq = kChirp;  break;
    case BuzzPattern::Double: s_seq = kDouble; break;
    case BuzzPattern::Long:   s_seq = kLong;   break;
    case BuzzPattern::Alarm:  s_seq = kAlarm; s_repeat = true; break;
    default: s_seq = nullptr; toneOn(false); return;
  }
  s_idx = 0;
  s_segStart = millis();
  toneOn(s_seq[0].on);
}

void buzzer_update() {
  if (!active() || s_seq == nullptr) return;
  const Seg& seg = s_seq[s_idx];
  if (seg.ms == 0) {                 // terminal segment
    if (s_repeat) { s_idx = 0; s_segStart = millis(); toneOn(s_seq[0].on); }
    else          { s_seq = nullptr; toneOn(false); }
    return;
  }
  if (millis() - s_segStart >= seg.ms) {
    s_idx++;
    s_segStart = millis();
    toneOn(s_seq[s_idx].on);
  }
}

void buzzer_set_muted(bool m) { s_muted = m; if (m) { /* Alarm still allowed */ } }
bool buzzer_is_muted() { return s_muted; }

}  // namespace hal

#endif  // !PANPILOT_SIM
