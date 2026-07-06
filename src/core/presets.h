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

// --- Custom presets (Phase 2 preset editor) ---------------------------------
// User-defined presets live after the built-ins: ids PRESET_COUNT .. total()-1.
// warnF is derived (hi + 100, capped at the hard ceiling). Persisted by the HAL
// as an opaque blob. Hardware-free + unit-tested.
#define PRESET_NAME_MAX   18
#define PRESET_CUSTOM_MAX 8

int presets_total();                  // PRESET_COUNT + custom count
int presets_custom_count();
bool presets_is_custom(uint8_t id);

// Add a custom preset; returns its id, or -1 if full / invalid range.
int presets_add(const char* name, int loF, int hiF, bool stainless);
// Update an existing custom preset (id >= PRESET_COUNT); no-op otherwise.
void presets_update(uint8_t id, const char* name, int loF, int hiF, bool stainless);
// Remove a custom preset (later ids shift down by one).
void presets_remove(uint8_t id);

// Persistence: serialize/restore the custom table as one blob.
const void* presets_custom_blob();
uint32_t presets_custom_blob_bytes();
void presets_load_custom(const void* data, uint32_t bytes);
