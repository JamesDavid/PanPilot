// thermal_palette.h — ironbow-ish false-color map (base spec §9.2). Header-only,
// hardware-free (returns plain RGB), so both the device UI and the simulator use
// the identical palette. Input is a normalized 0..1 value (scene min..max).
#pragma once
#include <stdint.h>

struct RGB8 { uint8_t r, g, b; };

// Ironbow control points: black -> purple -> magenta -> red -> orange -> yellow
// -> white. Piecewise-linear interpolation.
inline RGB8 ironbow(float t) {
  if (t < 0) t = 0; else if (t > 1) t = 1;
  static const int N = 7;
  static const float stops[N] = {0.00f, 0.15f, 0.33f, 0.50f, 0.70f, 0.88f, 1.00f};
  static const RGB8 cols[N] = {
      {0, 0, 0}, {40, 0, 80}, {130, 0, 130}, {200, 30, 30},
      {240, 120, 0}, {255, 220, 40}, {255, 255, 255}};
  int i = 0;
  while (i < N - 1 && t > stops[i + 1]) ++i;
  const float span = stops[i + 1] - stops[i];
  const float f = span > 0 ? (t - stops[i]) / span : 0.0f;
  auto lerp = [](uint8_t a, uint8_t b, float k) {
    return static_cast<uint8_t>(a + (b - a) * k);
  };
  return {lerp(cols[i].r, cols[i + 1].r, f),
          lerp(cols[i].g, cols[i + 1].g, f),
          lerp(cols[i].b, cols[i + 1].b, f)};
}
