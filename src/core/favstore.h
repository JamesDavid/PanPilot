// favstore.h — favorite foods (bench 2026-07-12: "surface recipes to the top
// level menu for things they do a lot"). Up to 8 favorites, identified by an
// FNV-1a hash of "name|variant" so they survive firmware updates that reorder
// the food database (index-based ids would not). Starred foods sort first in
// the food picker and appear as one-tap cards at the top of the preset picker.
// Hardware-free + unit-tested; persisted as an opaque blob (NVS "favs1").
#pragma once
#include <stdint.h>
#include <cstring>

inline uint32_t fav_hash(const char* name, const char* variant) {
  uint32_t h = 2166136261u;
  for (const char* p = name; p && *p; ++p) { h ^= (uint8_t)*p; h *= 16777619u; }
  h ^= (uint8_t)'|'; h *= 16777619u;
  for (const char* p = variant; p && *p; ++p) { h ^= (uint8_t)*p; h *= 16777619u; }
  return h;
}

class FavStore {
 public:
  static constexpr int MAX = 8;

  int count() const { return n_; }
  bool has(uint32_t h) const {
    for (int i = 0; i < n_; ++i)
      if (h_[i] == h) return true;
    return false;
  }
  // Toggle a favorite. Returns the NEW state; an add is refused (false) when
  // the list is full so the UI can say "unstar something first".
  bool toggle(uint32_t h) {
    for (int i = 0; i < n_; ++i)
      if (h_[i] == h) {
        for (int k = i; k < n_ - 1; ++k) h_[k] = h_[k + 1];
        --n_;
        return false;
      }
    if (n_ >= MAX) return false;
    h_[n_++] = h;
    return true;
  }

  // --- persistence (opaque u32 array) ---
  const void* blob() const { return h_; }
  uint32_t blobBytes() const { return (uint32_t)(n_ * sizeof(uint32_t)); }
  void loadBlob(const void* d, uint32_t bytes) {
    int c = (int)(bytes / sizeof(uint32_t));
    if (c > MAX) c = MAX;
    if (c < 0) c = 0;
    if (c > 0) std::memcpy(h_, d, (size_t)c * sizeof(uint32_t));
    n_ = c;
  }

 private:
  uint32_t h_[MAX] = {0};
  int n_ = 0;
};
