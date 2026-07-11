// presets.cpp — see presets.h. Values are consensus skillet/griddle surface
// temperatures consistent with the Appendix A food database (spec §2.7).
#include "presets.h"
#include <algorithm>
#include <cstring>

namespace {
const Preset kPresets[PRESET_COUNT] = {
  //  name              lo   hi   warn  recov  stainless
  { "Eggs",            270, 300,  360,  false, false },
  { "Pancakes",        350, 375,  440,  true,  false },
  { "Sear",            475, 550,  650,  true,  false },
  { "Tortillas",       400, 450,  520,  false, false },
  { "Generic",         340, 360,  450,  false, false },
};

// Custom presets. Stored data is POD (serializable); the parallel Preset views
// keep name pointing into the stored buffer so preset() can return a reference.
struct CustomData {
  char name[PRESET_NAME_MAX + 1];
  int16_t loF, hiF, warnF;
  uint8_t stainless;
};
CustomData g_data[PRESET_CUSTOM_MAX];
Preset g_view[PRESET_CUSTOM_MAX];
int g_customN = 0;

int clampWarn(int hi) {
  const int w = hi + 100;
  return w > 650 ? 650 : w;
}
void rebuild_views() {
  for (int i = 0; i < g_customN; ++i) {
    g_view[i].name = g_data[i].name;
    g_view[i].loF = g_data[i].loF;
    g_view[i].hiF = g_data[i].hiF;
    g_view[i].warnF = g_data[i].warnF;
    g_view[i].recoveryMonitor = true;   // user presets get recovery monitoring
    g_view[i].stainlessHints = g_data[i].stainless != 0;
  }
}
void fill(CustomData& d, const char* name, int loF, int hiF, bool stainless) {
  std::strncpy(d.name, name ? name : "Custom", PRESET_NAME_MAX);
  d.name[PRESET_NAME_MAX] = '\0';
  if (d.name[0] == '\0') std::strcpy(d.name, "Custom");
  d.loF = (int16_t)loF;
  d.hiF = (int16_t)hiF;
  d.warnF = (int16_t)clampWarn(hiF);
  d.stainless = stainless ? 1 : 0;
}
}  // namespace

const Preset& preset(uint8_t id) {
  if (id < PRESET_COUNT) return kPresets[id];
  const int ci = id - PRESET_COUNT;
  if (ci >= 0 && ci < g_customN) return g_view[ci];
  return kPresets[PRESET_GENERIC];
}

int presets_total() { return PRESET_COUNT + g_customN; }
int presets_custom_count() { return g_customN; }
bool presets_is_custom(uint8_t id) { return id >= PRESET_COUNT && id < presets_total(); }

int presets_add(const char* name, int loF, int hiF, bool stainless) {
  if (g_customN >= PRESET_CUSTOM_MAX) return -1;
  if (hiF <= loF) return -1;
  fill(g_data[g_customN], name, loF, hiF, stainless);
  ++g_customN;
  rebuild_views();
  return PRESET_COUNT + g_customN - 1;
}

void presets_update(uint8_t id, const char* name, int loF, int hiF, bool stainless) {
  if (!presets_is_custom(id) || hiF <= loF) return;
  fill(g_data[id - PRESET_COUNT], name, loF, hiF, stainless);
  rebuild_views();
}

void presets_remove(uint8_t id) {
  if (!presets_is_custom(id)) return;
  const int ci = id - PRESET_COUNT;
  for (int i = ci; i < g_customN - 1; ++i) g_data[i] = g_data[i + 1];
  --g_customN;
  rebuild_views();
}

const void* presets_custom_blob() { return g_data; }
uint32_t presets_custom_blob_bytes() {
  return (uint32_t)(g_customN * (int)sizeof(CustomData));
}
void presets_load_custom(const void* data, uint32_t bytes) {
  int cnt = (int)(bytes / sizeof(CustomData));
  if (cnt > PRESET_CUSTOM_MAX) cnt = PRESET_CUSTOM_MAX;
  if (cnt < 0) cnt = 0;
  if (cnt > 0) std::memcpy(g_data, data, (size_t)cnt * sizeof(CustomData));
  g_customN = cnt;
  rebuild_views();
}

Target preset_target(uint8_t id) {
  const Preset& p = preset(id);
  Target t;
  t.loF = p.loF; t.hiF = p.hiF; t.warnF = p.warnF;
  t.centerF = (p.loF + p.hiF) / 2;
  return t;
}
