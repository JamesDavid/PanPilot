// multipan.h — track the top-2 pan blobs with persistent zone IDs (roadmap
// §2.4). The 32×24 array can see two adjacent burners; this finds both and keeps
// each pinned to a stable zone (0 = primary, 1 = secondary) frame-to-frame by
// nearest-centroid association, so per-zone guidance stays attached to the right
// pan. Hardware-free + unit-tested. (Blob detection mirrors frame_analysis §6.1;
// kept separate so the single-pan path stays untouched.)
#pragma once
#include "pan_types.h"
#include "app_config.h"

class MultiPanTracker {
 public:
  // Fill out[0]=primary, out[1]=secondary (by persistent zone). Returns the
  // number of pans currently tracked (0..2). Unused zones get presence=ABSENT.
  int process(const ThermalFrame& f, PanReading out[2]);
  void reset();

 private:
  bool have_[2] = {false, false};
  float cx_[2] = {0, 0}, cy_[2] = {0, 0};
  uint8_t miss_[2] = {0, 0};   // consecutive frames a zone went unmatched
};
