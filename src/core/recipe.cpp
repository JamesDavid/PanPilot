// recipe.cpp — see recipe.h.
#include "recipe.h"
#include "core/preplib.h"
#include <cmath>

void RecipeEngine::enter(int i, uint32_t now) {
  idx_ = i;
  entered_ = true;
  step_start_ = now;
}

void RecipeEngine::start(const RecipeProgram* p, uint32_t now) {
  prog_ = p; finished_ = false; loops_left_ = -1;
  enter(0, now);
}

RecipeOut RecipeEngine::step(const RecipeInput& in) {
  RecipeOut o;
  if (!prog_) return o;
  if (finished_) { o.finished = true; return o; }

  for (int guard = 0; guard < 64; ++guard) {   // resolve instantaneous steps
    const RecipeStep& s = prog_->steps[idx_];
    o = RecipeOut{};
    o.active = true; o.stepIndex = idx_; o.type = s.type; o.cue = s.say;
    bool advance = false; int jumpTo = -1;

    switch (s.type) {
      case StepType::HOLD:
        o.setpointF = s.a;
        if (std::fabs(in.panTempF - s.a) <= 15.0f) advance = true;   // "ready"
        break;
      case StepType::CUE:
        if (((s.expect & EXP_FOOD_ADDED) && in.foodAdded) ||
            ((s.expect & EXP_TOUCH) && in.touch) ||
            (s.b > 0 && in.now - step_start_ >= (uint32_t)s.b * 1000))
          advance = true;
        break;
      case StepType::TIMER:
        if (in.now - step_start_ >= (uint32_t)s.a * 1000) advance = true;
        break;
      case StepType::PREP: {
        const PrepEntry& p = preplib_entry(s.a);
        o.cue = p.name;
        if (in.panTempF > p.maxAddTempF) {           // butter on a 425° pan burns
          o.prepTooHot = true;
          o.setpointF = p.addTempF_hi;               // cool to the add window
        } else if (in.touch) {
          advance = true;                            // confirmed added/melted
        }
        break;
      }
      case StepType::LOOP:
        if (loops_left_ < 0) loops_left_ = s.b;
        if (loops_left_ > 0) { --loops_left_; jumpTo = s.a; }
        else { loops_left_ = -1; advance = true; }
        break;
      case StepType::END:
        finished_ = true; o.finished = true; o.active = false;
        return o;
    }

    if (jumpTo >= 0) { enter(jumpTo, in.now); continue; }
    if (advance) {
      if (idx_ + 1 >= prog_->nSteps) { finished_ = true; o.finished = true; o.active = false; return o; }
      enter(idx_ + 1, in.now);
      continue;
    }
    return o;   // still waiting on this step
  }
  return o;
}

// -------- built-in program: Smash Burgers x4 (roadmap §4.1 example) ----------
static const RecipeStep kSmash[] = {
  { StepType::HOLD,  450, 0, EXP_NONE, "Preheat to 450" },
  { StepType::CUE,     0, 120, EXP_FOOD_ADDED, "Add 2 patties + smash" },
  { StepType::TIMER,  90, 0, EXP_NONE, "Searing side 1" },
  { StepType::CUE,     0, 0, (uint8_t)(EXP_FOOD_ADDED | EXP_TOUCH), "Flip + cheese" },
  { StepType::TIMER,  60, 0, EXP_NONE, "Finishing" },
  { StepType::CUE,     0, 0, EXP_TOUCH, "Remove. Pan recovering for next batch" },
  { StepType::LOOP,    1, 3, EXP_NONE, "" },      // back to step 1, 3 more batches
  { StepType::END,     0, 0, EXP_NONE, "" },
};
static const RecipeProgram kSmashProg = { "Smash Burgers x4", kSmash,
                                          (int)(sizeof(kSmash) / sizeof(kSmash[0])) };

const RecipeProgram* recipe_builtin_smashburger() { return &kSmashProg; }
