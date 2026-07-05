// presets.h — built-in cooking presets (base spec §7.3 / parent §19). Each maps
// to a Target band + warn threshold plus recovery-monitor and stainless flags.
// Hardware-free; the picker UI and guidance both read this table.
#pragma once
#include <stdint.h>
#include "core/guidance.h"

struct Preset {
  const char* name;
  int loF, hiF, warnF;
  bool recoveryMonitor;   // watch for FOOD ADDED -> recovery (§7.4, M6)
  bool stainlessHints;    // show the bare-stainless banner (§7.5)
};

// Order is stable (persisted by index). Generic is last.
enum PresetId : uint8_t {
  PRESET_EGGS, PRESET_PANCAKES, PRESET_STAINLESS, PRESET_SEAR,
  PRESET_TORTILLAS, PRESET_GENERIC, PRESET_COUNT
};

const Preset& preset(uint8_t id);
Target preset_target(uint8_t id);     // Preset -> Target band
