// profilestore.h — up to 8 named Learn-Pan profiles with an active selection
// (base spec §7 Phase 1.5 / §10; Phase 3 "8 profiles, not 1"). Each profile is
// a learned thermal lag for a specific pan+burner; the active one feeds the
// overshoot predictor. Hardware-free + unit-tested; the device persists it as a
// blob plus the active index.
#pragma once
#include <cstring>
#include <stdint.h>
#include "core/profiles.h"

class ProfileStore {
 public:
  static constexpr int MAX = 8;

  int count() const { return n_; }
  int active() const { return active_; }
  void setActive(int i) { if (i >= 0 && i < n_) active_ = i; }
  const PanProfile& at(int i) const { return p_[i]; }
  const PanProfile* activeProfile() const {
    return (active_ >= 0 && active_ < n_) ? &p_[active_] : nullptr;
  }

  // Add a profile and make it active. Returns its index, or -1 if full.
  int add(const PanProfile& pr) {
    if (n_ >= MAX) return -1;
    p_[n_] = pr;
    active_ = n_;
    return n_++;
  }
  void remove(int i) {
    if (i < 0 || i >= n_) return;
    for (int k = i; k < n_ - 1; ++k) p_[k] = p_[k + 1];
    --n_;
    if (active_ >= n_) active_ = n_ - 1;   // -1 when empty
  }
  void rename(int i, const char* name) {
    if (i < 0 || i >= n_ || !name) return;
    std::strncpy(p_[i].name, name, sizeof(p_[i].name) - 1);
    p_[i].name[sizeof(p_[i].name) - 1] = '\0';
  }

  // --- persistence ---
  const void* blob() const { return p_; }
  uint32_t blobBytes() const { return (uint32_t)(n_ * (int)sizeof(PanProfile)); }
  void loadBlob(const void* d, uint32_t bytes, int active) {
    int c = (int)(bytes / sizeof(PanProfile));
    if (c > MAX) c = MAX;
    if (c < 0) c = 0;
    if (c > 0) std::memcpy(p_, d, (size_t)c * sizeof(PanProfile));
    n_ = c;
    active_ = (active >= 0 && active < c) ? active : (c > 0 ? 0 : -1);
  }

 private:
  PanProfile p_[MAX];
  int n_ = 0;
  int active_ = -1;
};
