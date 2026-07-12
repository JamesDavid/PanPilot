// =============================================================================
// foodlib_user.h — user-requested food additions (James, bench 2026-07-12).
//
// Appendix A (foodlib_seed.h) is spec-verbatim and must not be edited, so
// additions live here and are merged after the seed (core/foodlib.cpp). Same
// units/conventions as the seed header.
//
// DONENESS LADDER: James cooks meats MEDIUM-WELL to WELL, so every meat with
// a doneness spectrum gets those variants (times extrapolated from the seed's
// rare -> medium progression, ~+45 s/side per level; well-done steak drops the
// band slightly so the crust doesn't char before the center catches up).
// safeInternalF stays at the USDA floor (145 whole-muscle / 160 ground) —
// it is the SAFETY line on the REMOVE cue, not the doneness target; chicken
// needs no variants because 165 is already fully cooked.
//
// DATA PROVENANCE: consensus skillet charts (same sources as the seed),
// midpoint-of-band; the ±8% feedback loop personalizes. All [REVIEW] pending
// James's bench cooks, same as the seed's lower-confidence rows.
// =============================================================================
#pragma once
#include "core/foodlib/foodlib_seed.h"

static const FoodEntry FOODLIB_USER[] = {

// ---------------------------------------------------------------- Steak
{ "Steak", "1-inch, medium well  [REVIEW]",
  450, 550, {285, 250, 0, 0}, 2, 500, -10, 300, 145,
  "Flip once when crust releases freely; rest is mandatory" },

{ "Steak", "1-inch, well done  [REVIEW]",
  425, 500, {330, 290, 0, 0}, 2, 463, -10, 300, 145,
  "Slightly cooler pan, longer cook - crust shouldn't char" },

// ---------------------------------------------------------------- Burgers
{ "Burger", "1/3 lb, medium well  [REVIEW]",
  375, 425, {270, 210, 0, 0}, 2, 400, -12, 120, 160,
  "Flip once, don't press - juices stay in" },

{ "Burger", "1/3 lb, well done  [REVIEW]",
  375, 425, {300, 240, 0, 0}, 2, 400, -12, 120, 160,
  "Flip once; firm all over when fully done" },

// ---------------------------------------------------------------- Pork
{ "Pork chop", "3/4-inch, medium well  [REVIEW]",
  375, 400, {255, 225, 0, 0}, 2, 388, -10, 180, 145,
  "Flip when well browned; slight give at the center" },

{ "Pork chop", "3/4-inch, well done  [REVIEW]",
  350, 375, {300, 270, 0, 0}, 2, 363, -10, 180, 145,
  "Flip when well browned; cook until firm, juices run clear" },

{ "Pork tenderloin", "1-inch medallions, med-well  [REVIEW]",
  375, 400, {240, 210, 0, 0}, 2, 388, -10, 180, 145,
  "Flip when deeply browned; firm with slight give = med-well" },

{ "Pork tenderloin", "1-inch medallions, well done  [REVIEW]",
  375, 400, {285, 255, 0, 0}, 2, 388, -10, 180, 145,
  "Flip when deeply browned; cook until uniformly firm" },

// ---------------------------------------------------------------- Ground
{ "Ground beef", "crumbles, browned  [REVIEW]",
  350, 400, {300, 240, 0, 0}, 2, 375, -8, 0, 160,
  "Break up and stir often; 'flip' cue = give it a full turn" },

// ------------------------------------------------------------- Multi-flip
// James flips every ~2 min rather than once (bench 2026-07-12) — a legit
// technique (more even cooking). Same total heat load as the flip-once rows,
// split into 4 turns so the FLIP cue fires on his cadence.
{ "Chicken breast", "butterflied, flip every 2 min  [REVIEW]",
  350, 400, {120, 120, 120, 90}, 4, 375, -10, 180, 165,
  "Turn on each cue; 165°F internal is non-negotiable" },

{ "Pork chop", "3/4-inch med-well, flip every 2 min  [REVIEW]",
  375, 400, {120, 120, 120, 120}, 4, 388, -10, 180, 145,
  "Turn on each cue; firm with slight give when done" },

};

#define FOODLIB_USER_COUNT (sizeof(FOODLIB_USER) / sizeof(FOODLIB_USER[0]))
