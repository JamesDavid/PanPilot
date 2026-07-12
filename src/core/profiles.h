// profiles.h — Learn Pan Mode data model (base spec §7 Phase 1.5, §9.3, §10).
// A PanProfile captures how a specific pan+burner behaves so guidance can use a
// learned thermal lag (for overshoot prediction) instead of the default.
// Hardware-free; the learn heuristic is unit-tested.
#pragma once
#include <stdint.h>
#include "app_config.h"

struct PanProfile {
  uint16_t magic;            // 0xPA71 sentinel (base spec §10 versioned blob)
  uint16_t version;
  char     name[16];
  float    peakRateFPerMin;  // fastest observed heating rate
  float    lagMinutes;       // learned thermal lag (overshoot predictor)
  bool     valid;
  bool     stainless;        // pan MATERIAL (v2): the selected pan drives the
                             // stainless trend-only/alarm-suppression behavior
};

constexpr uint16_t PANPROFILE_MAGIC = 0xA71C;
constexpr uint16_t PANPROFILE_VERSION = 2;   // v2: + stainless (struct grew)

// Heuristic: a faster-heating pan/burner overshoots further, so it has a larger
// effective lag. Derived from the peak heating rate observed during Learn Pan
// Mode; clamped to a sane band. (Refined later by measuring post-cutoff drift.)
float learn_lag_from_rate(float peakRateFPerMin);

PanProfile make_profile(const char* name, float peakRateFPerMin,
                        bool stainless = false);
