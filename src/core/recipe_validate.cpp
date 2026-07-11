// recipe_validate.cpp — see recipe_validate.h.
#include "recipe_validate.h"
#include "core/preplib.h"
#include "app_config.h"

RecipeVerdict recipe_validate(const RecipeStep* steps, int n, bool brownOnPurpose) {
  RecipeVerdict v;
  auto fail = [&](int i, const char* r) { v.ok = false; v.badStep = i; v.reason = r; };

  if (n <= 0) { fail(-1, "empty program"); return v; }
  if (n > 40) { fail(-1, "too many steps (max 40)"); return v; }

  int panFat = -1;   // prepId of the fat currently in the pan (-1 = none)
  for (int i = 0; i < n; ++i) {
    const RecipeStep& s = steps[i];
    switch (s.type) {
      case StepType::HOLD:
        if (s.a > (int)ABS_MAX_TEMP_F)
          { fail(i, "hold above the 650F ceiling"); return v; }
        if (panFat >= 0 && !brownOnPurpose &&
            preplib_entry(panFat).smokePointF > 0 &&
            s.a > (int)preplib_entry(panFat).smokePointF - 25)
          { fail(i, "hold exceeds the in-pan fat's smoke point"); return v; }
        break;
      case StepType::PREP:
        if (s.a < 0 || s.a >= preplib_count())
          { fail(i, "unknown prep ingredient"); return v; }
        panFat = s.a;
        break;
      case StepType::CUE:
        if (s.expect == EXP_NONE && s.b <= 0)
          { fail(i, "cue can hang forever (needs an event or timeout)"); return v; }
        break;
      case StepType::LOOP:
        if (s.a < 0 || s.a >= i) { fail(i, "loop must point to an earlier step"); return v; }
        if (s.b < 1 || s.b > 20) { fail(i, "loop count must be 1..20"); return v; }
        // The engine has ONE loop counter: a LOOP whose body contains another
        // LOOP never terminates (the inner one steals + resets the counter).
        for (int j = s.a; j < i; ++j)
          if (steps[j].type == StepType::LOOP)
            { fail(i, "nested loops are not supported"); return v; }
        break;
      default: break;
    }
  }
  return v;
}
