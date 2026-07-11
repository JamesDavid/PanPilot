// recipe.cpp — see recipe.h.
#include "recipe.h"
#include "core/preplib.h"
#include <cmath>

void RecipeEngine::enter(int i, uint32_t now) {
  idx_ = i;
  entered_ = true;
  step_start_ = now;
  in_window_ = false;         // reset PREP add-window dwell for the new step
  window_enter_ = now;
}

void RecipeEngine::start(const RecipeProgram* p, uint32_t now) {
  prog_ = p; finished_ = false; loops_left_ = -1; fat_clamp_ = 0;
  enter(0, now);
}

RecipeOut RecipeEngine::step(const RecipeInput& in) {
  RecipeOut o;
  if (!prog_) return o;
  if (finished_) { o.finished = true; o.fatClampWarnF = fat_clamp_; return o; }

  // One physical tap / food-drop satisfies at most ONE gated step: consume the
  // event when it advances a step so it can't blow through several consecutive
  // touch/food-gated steps inside the resolution loop below.
  bool touch = in.touch;
  bool foodAdded = in.foodAdded;

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
        if ((s.expect & EXP_FOOD_ADDED) && foodAdded) { advance = true; foodAdded = false; }
        else if ((s.expect & EXP_TOUCH) && touch)     { advance = true; touch = false; }
        else if (s.b > 0 && in.now - step_start_ >= (uint32_t)s.b * 1000)
          advance = true;
        break;
      case StepType::TIMER:
        if (in.now - step_start_ >= (uint32_t)s.a * 1000) advance = true;
        break;
      case StepType::PREP: {
        const PrepEntry& p = preplib_entry(s.a);
        // Track continuous dwell in the fat's add window (melt-detection).
        const bool inWin = in.panTempF >= (float)p.addTempF_lo &&
                           in.panTempF <= (float)p.maxAddTempF;
        if (inWin && !in_window_) { in_window_ = true; window_enter_ = in.now; }
        else if (!inWin) in_window_ = false;
        const uint32_t dwell = in_window_ ? (in.now - window_enter_) : 0;

        PrepStatus ps = prep_evaluate(s.a, in.panTempF, in.rateFPerMin, dwell,
                                      prog_->brownOnPurpose);
        // Latch the smoke-point clamp for the rest of the program (min across
        // fats). Latched at evaluation, not at confirm — conservative, and it
        // survives the PREP advancing within this same step() call.
        if (ps.fatClampWarnF > 0)
          fat_clamp_ = fat_clamp_ ? (ps.fatClampWarnF < fat_clamp_
                                         ? ps.fatClampWarnF : fat_clamp_)
                                  : ps.fatClampWarnF;
        o.prepTooHot = ps.tooHot;
        o.prepBelowWindow = ps.belowWindow;
        o.prepReady = ps.ready;
        o.prepAdvice = ps.advice;
        if (ps.tooHot) {
          o.setpointF = ps.coolToF;                  // cool to the add window
          o.cue = "Pan too hot - cooling for the fat";
        } else if (ps.belowWindow) {
          o.cue = "Heating - add the fat soon";
        } else if (ps.ready) {
          o.cue = p.name;                            // "Butter" -> add it now
        } else {
          o.cue = "Almost ready for the fat";
        }
        if (touch) { advance = true; touch = false; }  // human confirms it's in
        break;
      }
      case StepType::LOOP:
        if (loops_left_ < 0) loops_left_ = s.b;
        if (loops_left_ > 0) { --loops_left_; jumpTo = s.a; }
        else { loops_left_ = -1; advance = true; }
        break;
      case StepType::END:
        finished_ = true; o.finished = true; o.active = false;
        o.fatClampWarnF = fat_clamp_;
        return o;
    }

    if (jumpTo >= 0) { enter(jumpTo, in.now); continue; }
    if (advance) {
      if (idx_ + 1 >= prog_->nSteps) {
        finished_ = true; o.finished = true; o.active = false;
        o.fatClampWarnF = fat_clamp_;
        return o;
      }
      enter(idx_ + 1, in.now);
      continue;
    }
    o.fatClampWarnF = fat_clamp_;   // carried every tick, not only on PREP ticks
    return o;                       // still waiting on this step
  }
  o.fatClampWarnF = fat_clamp_;
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
                                          (int)(sizeof(kSmash) / sizeof(kSmash[0])),
                                          /*brownOnPurpose=*/false };

const RecipeProgram* recipe_builtin_smashburger() { return &kSmashProg; }
