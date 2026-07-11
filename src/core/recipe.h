// recipe.h — declarative cook-program sequencer (roadmap §4.1, Track B).
// Phase-3-independent: in Advisor mode a HOLD step cues a knob change, in
// Autopilot it sets a controller setpoint — the sequencer is identical. Each
// step arms expected perception events, runs timers, and cues the human.
// Hardware-free + unit-tested. JSON load/save (LittleFS) + the web Recipe
// Creator (M20) build these programs; the engine just runs them.
#pragma once
#include <stdint.h>

enum class StepType : uint8_t { HOLD, CUE, TIMER, PREP, LOOP, END };

// Expected perception events that advance a CUE step.
enum : uint8_t {
  EXP_NONE = 0, EXP_FOOD_ADDED = 1, EXP_TOUCH = 2, EXP_TIMER = 4,
};

// Plain aggregate (no default member initializers) so brace-init arrays work
// under the device's C++ standard as well as the host's.
struct RecipeStep {
  StepType type;
  int a;               // HOLD: setpoint °F | TIMER: sec | PREP: prepId
                       // LOOP: target step index
  int b;               // LOOP: loop count | CUE: timeout sec (0 = none)
  uint8_t expect;      // CUE: which event(s) advance it
  const char* say;     // cue text
};

struct RecipeProgram {
  const char* name;
  const RecipeStep* steps;
  int nSteps;
  bool brownOnPurpose;   // relax the fat smoke-point clamp (author opted in)
};

struct RecipeInput {
  float panTempF;
  float rateFPerMin = 0;   // for PREP equalize-detection (roadmap §4.1.1)
  bool foodAdded = false;
  bool touch = false;
  uint32_t now = 0;
};

struct RecipeOut {
  bool active = false;
  bool finished = false;
  int stepIndex = 0;
  StepType type = StepType::END;
  int setpointF = 0;       // current HOLD setpoint (0 = none)
  const char* cue = "";    // instruction for the human ("" = none)
  bool prepTooHot = false; // PREP: pan too hot for this fat -> cool first
  // PREP fat monitor (roadmap §4.1.1):
  bool prepReady = false;      // fat is in the add window and ready to add
  bool prepBelowWindow = false;// pan still too cool to add the fat
  // Overheat clamp while fat is in the pan: LATCHED by the engine from the
  // first PREP evaluation onward (min smoke clamp across all fats used), and
  // carried on EVERY tick's output — a PREP that confirms-and-advances within
  // one step() call used to drop it. 0 = no fat in play.
  int fatClampWarnF = 0;
  const char* prepAdvice = ""; // the fat's ready hint
};

class RecipeEngine {
 public:
  void start(const RecipeProgram* p, uint32_t now);
  RecipeOut step(const RecipeInput& in);
  bool active() const { return prog_ && !finished_; }

 private:
  const RecipeProgram* prog_ = nullptr;
  int idx_ = 0;
  bool finished_ = false;
  bool entered_ = false;
  uint32_t step_start_ = 0;
  int loops_left_ = -1;     // for the current LOOP step
  bool in_window_ = false;  // PREP: pan currently in the fat's add window
  uint32_t window_enter_ = 0;
  int fat_clamp_ = 0;       // latched fat smoke clamp (see RecipeOut)

  void enter(int i, uint32_t now);
};

const RecipeProgram* recipe_builtin_smashburger();   // roadmap §4.1 example
