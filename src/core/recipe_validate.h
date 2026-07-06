// recipe_validate.h — recipe program validation (roadmap §4.1.1). The firmware
// validator is the single source of truth; the browser mirrors these rules for
// instant feedback but the device re-validates on save. Hardware-free +
// unit-tested. Rules enforced here:
//   - <= 40 steps
//   - every HOLD <= 650 °F (interlock S5 ceiling)
//   - LOOP target points to an earlier step; loop count 1..20
//   - every CUE has an expected event and/or a timeout (can't hang forever)
//   - a PREP fat is not followed by a HOLD above its smoke point - 25 °F
//     (unless the program is stamped "browning on purpose")
#pragma once
#include "core/recipe.h"

struct RecipeVerdict {
  bool ok = true;
  int badStep = -1;
  const char* reason = "";
};

RecipeVerdict recipe_validate(const RecipeStep* steps, int n,
                              bool brownOnPurpose = false);
