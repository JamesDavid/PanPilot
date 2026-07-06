// foodstore.h — runtime store for user custom foods & overrides (spec §2.7).
// The seed database is compiled into flash; the device parses /foods.json from
// LittleFS into a FoodStore, and foodlib merges it over the seed (matching
// (name,variant) shadows a seed entry; new pairs are appended). Hardware-free +
// unit-tested; each entry owns its string storage so the FoodEntry views stay
// valid for the life of the store.
#pragma once
#include <stdint.h>
#include <cstring>
#include "core/foodlib/foodlib_seed.h"

class FoodStore {
 public:
  static constexpr int MAX = 16;
  static constexpr int NAME = 24, VARIANT = 40, FLIP = 96;

  void clear() { n_ = 0; }
  int count() const { return n_; }
  const FoodEntry& entry(int i) const { return view_[i]; }

  // Add one custom food/override. sideSec must hold `sides` values (<= 4).
  // Returns false if full or the band is malformed.
  bool add(const char* name, const char* variant, int loF, int hiF,
           const uint16_t* sideSec, int sides, int refF, int compPct,
           int restSec, int safeInternalF, const char* flip) {
    if (n_ >= MAX) return false;
    if (hiF <= loF || sides < 1 || sides > FOODLIB_MAX_SIDES) return false;
    Data& d = data_[n_];
    copy(d.name, name ? name : "Custom", NAME);
    copy(d.variant, variant ? variant : "", VARIANT);
    copy(d.flip, flip ? flip : "", FLIP);
    FoodEntry& e = view_[n_];
    e.name = d.name; e.variant = d.variant; e.flipHint = d.flip;
    e.panTargetF_lo = (uint16_t)loF; e.panTargetF_hi = (uint16_t)hiF;
    for (int s = 0; s < FOODLIB_MAX_SIDES; ++s)
      e.sideSec[s] = (s < sides && sideSec) ? sideSec[s] : 0;
    e.sides = (uint8_t)sides;
    e.refTempF = (uint16_t)refF;
    e.tempCompPctPer25F = (int8_t)compPct;
    e.restSec = (uint16_t)restSec;
    e.safeInternalF = (uint16_t)safeInternalF;
    ++n_;
    return true;
  }

  const FoodEntry* find(const char* name, const char* variant) const {
    for (int i = 0; i < n_; ++i)
      if (std::strcmp(view_[i].name, name) == 0 &&
          std::strcmp(view_[i].variant, variant) == 0)
        return &view_[i];
    return nullptr;
  }

 private:
  struct Data { char name[NAME], variant[VARIANT], flip[FLIP]; };
  static void copy(char* dst, const char* src, int cap) {
    std::strncpy(dst, src, cap - 1);
    dst[cap - 1] = '\0';
  }
  Data data_[MAX];
  FoodEntry view_[MAX];
  int n_ = 0;
};
