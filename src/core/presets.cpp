// presets.cpp — see presets.h. Values are consensus skillet/griddle surface
// temperatures consistent with the Appendix A food database (spec §2.7).
#include "presets.h"
#include <algorithm>

namespace {
const Preset kPresets[PRESET_COUNT] = {
  //  name              lo   hi   warn  recov  stainless
  { "Eggs",            270, 300,  360,  false, false },
  { "Pancakes",        350, 375,  440,  true,  false },
  { "Stainless",       400, 450,  520,  false, true  },
  { "Sear",            475, 550,  650,  true,  false },
  { "Tortillas",       400, 450,  520,  false, false },
  { "Generic",         340, 360,  450,  false, false },
};
}  // namespace

const Preset& preset(uint8_t id) {
  if (id >= PRESET_COUNT) id = PRESET_GENERIC;
  return kPresets[id];
}

Target preset_target(uint8_t id) {
  const Preset& p = preset(id);
  Target t;
  t.loF = p.loF; t.hiF = p.hiF; t.warnF = p.warnF;
  t.centerF = (p.loF + p.hiF) / 2;
  return t;
}
