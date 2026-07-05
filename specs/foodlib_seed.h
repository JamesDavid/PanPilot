// =============================================================================
// foodlib_seed.h — PanPilot Cook-Time & Temperature Seed Database
// Implements spec §2.7 (panpilot-phase2-to-ultimate-spec.md, v1.1)
//
// UNITS / CONVENTIONS
//   - All temperatures °F (PAN SURFACE temperature, not air, not internal)
//   - All times in SECONDS, authored at refTempF
//   - sideSec[] entries beyond `sides` count must be 0
//   - sides: 1 = no flip, 2 = flip once, 3–4 = multi-turn (sausages)
//   - tempCompPctPer25F: % change in cook time per 25 °F pan deviation from
//     refTempF (negative = hotter pan cooks faster). Runtime clamps total
//     compensation to 0.7x–1.3x per spec.
//   - safeInternalF: USDA minimum internal temps — 165 poultry, 160 ground
//     meats, 145 whole-muscle pork/beef/fish (with rest). 0 = visual doneness,
//     no internal-temp claim. Any nonzero value forces the "verify internal
//     temp" line on the REMOVE cue. THIS FIELD IS SAFETY-CRITICAL; do not
//     zero it to reduce UI noise.
//
// DATA PROVENANCE
//   Times/temps are consensus ranges from mainstream published cooking
//   references and griddle/skillet charts, tuned to the midpoint of each
//   preset's target band. They are STARTING VALUES: the ±8% Under/Perfect/
//   Over feedback loop (spec §2.7) is expected to personalize them.
//   >>> HUMAN REVIEW REQUIRED before first release — James: eyeball every
//   >>> row, especially steak and poultry. Marked [REVIEW] where authoring
//   >>> confidence is lower (thickness-sensitive items).
//
// This table is compiled into flash. User overrides & custom foods live in
// LittleFS JSON (spec §2.7) and shadow entries by (name, variant) key.
// =============================================================================

#pragma once
#include <stdint.h>

#define FOODLIB_MAX_SIDES 4
#define FOODLIB_SCHEMA_VERSION 1

typedef struct {
    const char* name;
    const char* variant;
    uint16_t    panTargetF_lo;
    uint16_t    panTargetF_hi;
    uint16_t    sideSec[FOODLIB_MAX_SIDES];
    uint8_t     sides;
    uint16_t    refTempF;
    int8_t      tempCompPctPer25F;
    uint16_t    restSec;
    uint16_t    safeInternalF;
    const char* flipHint;
} FoodEntry;

static const FoodEntry FOODLIB_SEED[] = {

// ---------------------------------------------------------------- Breakfast
{ "Pancakes", "4-inch, standard batter",
  350, 375, {150,  90, 0, 0}, 2, 365, -12,   0,   0,
  "Flip when bubbles pop and stay open, edges look set" },

{ "Pancakes", "large / 6-inch",
  350, 375, {195, 120, 0, 0}, 2, 365, -12,   0,   0,
  "Flip when bubbles pop and stay open, edges look set" },

{ "French toast", "standard sliced bread",
  325, 350, {180, 150, 0, 0}, 2, 340, -12,   0,   0,
  "Flip when underside is deep golden" },

{ "Eggs", "sunny side up",
  275, 300, {180,   0, 0, 0}, 1, 285, -15,   0,   0,
  "Done when whites are fully set, yolk still soft — no flip" },

{ "Eggs", "over easy",
  275, 300, {120,  30, 0, 0}, 2, 285, -15,   0,   0,
  "Flip gently once whites are set; brief second side" },

{ "Eggs", "scrambled, soft",
  250, 300, {150,   0, 0, 0}, 1, 275, -15,   0,   0,
  "Stir/fold continuously; pull while slightly glossy — carryover finishes" },

{ "Omelette", "3-egg, folded",
  275, 325, {105,  30, 0, 0}, 2, 300, -15,   0,   0,
  "\"Flip\" = add filling and fold once top is nearly set" },

{ "Bacon", "regular cut",
  325, 350, {240, 180, 0, 0}, 2, 335, -12,   0,   0,
  "Flip when edges curl and underside is browned; go by look, not clock" },

{ "Bacon", "thick cut",
  325, 350, {300, 240, 0, 0}, 2, 335, -12,   0,   0,
  "Flip when edges curl and underside is browned" },

{ "Hash browns", "shredded, 1/2-inch layer",
  365, 400, {300, 240, 0, 0}, 2, 375, -10,   0,   0,
  "Flip once bottom crust is deep golden and holds together" },

{ "Sausage links", "fresh pork breakfast links",
  325, 350, {180, 180, 180, 180}, 4, 335, -10,   0, 160,
  "Quarter-turn each cue for even browning" },

// ---------------------------------------------------------------- Burgers
{ "Smash burger", "2 oz ball, smashed thin",
  450, 500, { 90,  45, 0, 0}, 2, 475, -10,   0, 160,
  "Scrape-flip when crust is deep brown; cheese on side 2" },

{ "Burger", "1/3 lb, 3/4-inch patty, medium",
  375, 425, {240, 180, 0, 0}, 2, 400, -12, 120, 160,
  "Flip once, don't press — juices stay in" },

// ---------------------------------------------------------------- Steak  [REVIEW]
// Whole-muscle beef: USDA 145 with 3-min rest. Times are strongly
// thickness-dependent; variants pin thickness. Pull temps for target
// doneness run below 145 by preference — device still shows the 145 note.
{ "Steak", "1-inch, rare",
  450, 550, {135, 120, 0, 0}, 2, 500, -10, 300, 145,
  "Flip once when crust releases freely; rest is mandatory" },

{ "Steak", "1-inch, medium rare",
  450, 550, {195, 165, 0, 0}, 2, 500, -10, 300, 145,
  "Flip once when crust releases freely; rest is mandatory" },

{ "Steak", "1-inch, medium",
  450, 550, {240, 210, 0, 0}, 2, 500, -10, 300, 145,
  "Flip once when crust releases freely; rest is mandatory" },

{ "Steak", "1.5-inch, medium rare  [REVIEW]",
  450, 550, {270, 240, 0, 0}, 2, 500, -10, 420, 145,
  "Consider finishing thick cuts with a probe — surface timing is a guide" },

// ---------------------------------------------------------------- Poultry
{ "Chicken breast", "butterflied / ~1/2-inch",
  350, 400, {240, 210, 0, 0}, 2, 375, -10, 180, 165,
  "Flip when underside is golden; 165°F internal is non-negotiable" },

{ "Chicken thigh", "boneless skinless",
  350, 400, {300, 240, 0, 0}, 2, 375, -10, 180, 165,
  "Thighs forgive overshoot; verify 165°F internal" },

// ---------------------------------------------------------------- Pork
{ "Pork chop", "3/4-inch boneless",
  400, 450, {210, 180, 0, 0}, 2, 425, -10, 180, 145,
  "Flip once; pull at 145°F internal + rest for juicy, not gray" },

// ---------------------------------------------------------------- Seafood
{ "Salmon", "1-inch fillet, skin-on",
  400, 450, {240, 120, 0, 0}, 2, 425, -12,  60, 145,
  "Start skin-down 80% of the cook; flip for a short finish" },

{ "White fish", "cod/tilapia, 3/4-inch",
  375, 425, {180, 120, 0, 0}, 2, 400, -12,   0, 145,
  "Flip carefully once underside releases; flakes when done" },

{ "Shrimp", "large, peeled",
  400, 450, { 90,  60, 0, 0}, 2, 425, -15,   0,   0,
  "Done at opaque and pink with a C-curl — overcooked at an O" },

// ---------------------------------------------------------------- Flattops & melts
{ "Grilled cheese", "standard, buttered",
  300, 325, {150, 120, 0, 0}, 2, 315, -15,   0,   0,
  "Low and slow: bread golden AND cheese melted, not one or the other" },

{ "Quesadilla", "single tortilla, folded",
  350, 375, {120,  90, 0, 0}, 2, 365, -12,   0,   0,
  "Flip when underside is blistered golden" },

{ "Tortillas", "warming, corn or flour",
  400, 450, { 30,  30, 0, 0}, 2, 425, -15,   0,   0,
  "Puffing is the done signal" },

// ---------------------------------------------------------------- Vegetarian
{ "Tofu", "extra-firm slabs, pressed",
  375, 425, {240, 180, 0, 0}, 2, 400, -10,   0,   0,
  "Don't touch until it releases on its own — that's the crust forming" },

{ "Smash potatoes", "par-boiled, smashed  [REVIEW]",
  375, 425, {300, 240, 0, 0}, 2, 400, -10,   0,   0,
  "Flip when edges are deep golden and crisp" },

};

#define FOODLIB_SEED_COUNT (sizeof(FOODLIB_SEED)/sizeof(FOODLIB_SEED[0]))

// -----------------------------------------------------------------------------
// Implementation notes for the timer engine (core/foodtimer):
//  - Doneness accumulator per spec §2.7: progress += dt * k(panTemp),
//    k = 1 + tempCompPctPer25F/100 * ((refTempF - panTempF)/25), clamp 0.7..1.3.
//    (Sign: pan hotter than ref -> k > 1 -> finishes sooner.)
//  - restSec runs after the REMOVE cue at attention level L1.
//  - safeInternalF != 0 appends: "Surface timing only — verify <T>°F internal"
//    to the REMOVE card. Never suppress.
//  - Feedback nudges (±8%) apply per (name, variant) in LittleFS overrides,
//    multiplicative on all sideSec, bounded to 0.6x–1.5x of seed lifetime.
// -----------------------------------------------------------------------------
