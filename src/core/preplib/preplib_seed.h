#pragma once
#include <stdint.h>

typedef enum { READY_MELTED, READY_EQUALIZED, READY_IMMEDIATE } PrepReady;

typedef struct {
    const char* name;
    uint16_t addTempF_lo;      // ideal add window (cue fires entering this band)
    uint16_t addTempF_hi;
    uint16_t maxAddTempF;      // above this: hold cue, cool pan first
    uint16_t smokePointF;      // runtime overheat clamp = this - 25
    PrepReady readyCriterion;  // thermal auto-advance signature
    const char* readyHint;     // sub-line on the cue card
} PrepEntry;

static const PrepEntry PREPLIB_SEED[] = {
// name                addLo addHi maxAdd smoke  ready              hint
{ "Butter",             225,  300,  325,  300, READY_MELTED,    "Foaming subsides = ready; solids brown fast past this" },
{ "Ghee / clarified",   250,  400,  450,  465, READY_MELTED,    "Melts clear; safe well past butter temps" },
{ "Olive oil (EVOO)",   250,  350,  375,  375, READY_EQUALIZED, "Shimmers and flows like water when ready" },
{ "Olive oil (light)",  250,  400,  440,  465, READY_EQUALIZED, "Shimmers when ready" },
{ "Canola/vegetable",   250,  400,  425,  400, READY_EQUALIZED, "Shimmers when ready; wisps of smoke = too far" },
{ "Avocado oil",        250,  475,  500,  500, READY_EQUALIZED, "The high-heat sear oil; shimmers when ready" },
{ "Peanut oil",         250,  425,  450,  450, READY_EQUALIZED, "Shimmers when ready" },
{ "Bacon fat",          225,  325,  350,  370, READY_MELTED,    "Melted and glossy = ready" },
{ "Coconut (refined)",  250,  375,  400,  400, READY_MELTED,    "Melts clear" },
{ "Water (steam/pot-stickers)", 212, 450, 550, 0, READY_IMMEDIATE, "Expect a loud sizzle and temp drop - lid on" },
};

#define PREPLIB_SEED_COUNT (sizeof(PREPLIB_SEED)/sizeof(PREPLIB_SEED[0]))

// Notes:
//  - smokePointF == 0 means no smoke clamp (water). Water instead suppresses
//    the recovery-rate alarm briefly: a steam drop is expected, not a fault.
//  - Runtime clamp while fat is in pan: warn = min(preset_warn, smokePointF-25),
//    unless the program carries the authors "browning on purpose" stamp
//    (then clamp = smokePointF).
//  - [REVIEW] all values before release, same policy as Appendix A.
