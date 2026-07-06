// attention.cpp — see attention.h.
#include "attention.h"
#include <cstring>

void AttentionManager::raise(AttnLevel level, const char* verb, const char* sub,
                             uint32_t now) {
  const bool same = active_ && level == level_ &&
                    strcmp(verb ? verb : "", verb_ ? verb_ : "") == 0;
  if (same) return;
  active_ = true;
  level_ = level;
  verb_ = verb ? verb : "";
  sub_ = sub ? sub : "";
  entered_ = true;            // fire entry beep on the next tick
  last_buzz_ = now;
}

void AttentionManager::ack(uint32_t) { clear(); }
void AttentionManager::clear() {
  active_ = false; level_ = AttnLevel::L0; verb_ = ""; sub_ = ""; entered_ = false;
}

AttnOutput AttentionManager::tick(uint32_t now) {
  AttnOutput o;
  if (!active_) return o;
  o.level = level_;
  o.verb = verb_;
  o.sub = sub_;
  o.strobe = (level_ == AttnLevel::L2 || level_ == AttnLevel::L3);

  // Decide whether a beep is due this tick.
  bool due = false;
  if (entered_) { due = true; entered_ = false; last_buzz_ = now; }
  else if (level_ == AttnLevel::L2 && now - last_buzz_ >= ATTN_L2_REPEAT_MS) {
    due = true; last_buzz_ = now;
  } else if (level_ == AttnLevel::L3 && now - last_buzz_ >= ATTN_L3_REPEAT_MS) {
    due = true; last_buzz_ = now;
  }
  // L0 is always silent; L1 beeps once on entry only.
  if (level_ == AttnLevel::L0) due = false;

  // Mute suppresses L0–L2 audio but NEVER L3 (roadmap §3.5).
  if (muted_ && level_ != AttnLevel::L3) due = false;

  o.buzz = due;
  return o;
}
