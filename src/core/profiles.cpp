// profiles.cpp — see profiles.h.
#include "profiles.h"
#include <algorithm>
#include <cstring>

float learn_lag_from_rate(float peakRateFPerMin) {
  if (peakRateFPerMin < 0) peakRateFPerMin = 0;
  const float lag = 0.3f + peakRateFPerMin / 200.0f;   // min per (°F/min)
  return std::max(0.3f, std::min(1.5f, lag));
}

PanProfile make_profile(const char* name, float peakRateFPerMin) {
  PanProfile p{};
  p.magic = PANPROFILE_MAGIC;
  p.version = PANPROFILE_VERSION;
  std::strncpy(p.name, name ? name : "Pan", sizeof(p.name) - 1);
  p.peakRateFPerMin = peakRateFPerMin;
  p.lagMinutes = learn_lag_from_rate(peakRateFPerMin);
  p.valid = true;
  return p;
}
