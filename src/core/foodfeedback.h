// foodfeedback.h — post-cook Under/Perfect/Over personalization (spec §2.7).
// After a cook the user grades the result; PanPilot nudges that food's cook
// time ±8% (multiplicative) and remembers it, so the seed database converges
// toward *your* stove. One factor per (name, variant), bounded 0.6x..1.5x of
// the seed lifetime. Hardware-free + unit-tested; the device persists the table
// as a single NVS blob (blob()/loadBlob()).
#pragma once
#include <stdint.h>
#include <string.h>

namespace panpilot {

class FeedbackStore {
 public:
  enum class Verdict : uint8_t { UNDER, PERFECT, OVER };
  static constexpr int MAX = 32;       // distinct (name,variant) foods tracked
  static constexpr float STEP = 0.08f; // ±8% per grade (spec §2.7)
  static constexpr float LO = 0.6f, HI = 1.5f;

  // Multiplier to apply to a food's seed side times. 1.0 (no change) if unseen.
  float factorFor(const char* name, const char* variant) const {
    const uint32_t k = key(name, variant);
    for (int i = 0; i < n_; ++i)
      if (items_[i].key == k) return items_[i].factor;
    return 1.0f;
  }

  // Record a grade. UNDER lengthens next time (+8%), OVER shortens (-8%),
  // PERFECT keeps the current factor. Result is clamped to [LO, HI].
  void apply(const char* name, const char* variant, Verdict v) {
    if (v == Verdict::PERFECT) return;   // current time was right — keep it
    float f = factorFor(name, variant);
    f *= (v == Verdict::UNDER) ? (1.0f + STEP) : (1.0f - STEP);
    if (f < LO) f = LO;
    if (f > HI) f = HI;
    set(key(name, variant), f);
  }

  int count() const { return n_; }

  // --- persistence (device: one NVS blob) ---
  const void* blob() const { return items_; }
  uint32_t blobBytes() const { return (uint32_t)(n_ * (int)sizeof(Item)); }
  void loadBlob(const void* p, uint32_t bytes) {
    int cnt = (int)(bytes / sizeof(Item));
    if (cnt > MAX) cnt = MAX;
    if (cnt < 0) cnt = 0;
    if (cnt > 0) memcpy(items_, p, (size_t)cnt * sizeof(Item));
    n_ = cnt;
    // Sanity-clamp loaded factors: apply() can only ever produce [LO, HI], so
    // anything outside (corrupt blob, NaN) resets to neutral rather than
    // driving a near-instant or never-ending cook timer.
    for (int i = 0; i < n_; ++i)
      if (!(items_[i].factor >= LO && items_[i].factor <= HI))
        items_[i].factor = 1.0f;
  }

 private:
  struct Item { uint32_t key; float factor; };
  Item items_[MAX];
  int n_ = 0;

  void set(uint32_t k, float f) {
    for (int i = 0; i < n_; ++i)
      if (items_[i].key == k) { items_[i].factor = f; return; }
    if (n_ < MAX) { items_[n_].key = k; items_[n_].factor = f; ++n_; }
  }

  static uint32_t fnv(const char* s, uint32_t h) {
    while (s && *s) { h ^= (uint8_t)*s++; h *= 16777619u; }
    return h;
  }
  static uint32_t key(const char* name, const char* variant) {
    uint32_t h = 2166136261u;
    h = fnv(name, h);
    h = fnv("|", h);
    h = fnv(variant, h);
    return h;
  }
};

}  // namespace panpilot
