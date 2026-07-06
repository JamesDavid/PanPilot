// app_state.h — the compact snapshot the UI renders (base spec §4 AppState,
// M2 subset). Hardware-free. Grows with guidance/ETA/preset at M3+.
#pragma once
#include <stdint.h>
#include "pan_types.h"
#include "core/thermal_model.h"
#include "core/guidance.h"
#include "core/presets.h"
#include "core/recovery.h"
#include "core/battery.h"
#include "core/foodtimer.h"

enum class Mode : uint8_t { THERMOMETER, TARGET, PRESET };
enum class LearnPhase : uint8_t { OFF, RECORDING, DONE };  // Learn Pan Mode (M6)

struct UiState {
  Mode mode = Mode::TARGET;
  PanPresence presence = PanPresence::ABSENT;
  bool  modelValid = false;
  float displayTempC = 0;
  float rateCPerMin = 0;
  Trend trend = Trend::STABLE;
  uint8_t confidence = 0;
  bool  moved = false;      // CHECK_AIM (base spec §6.4 MOVED)
  bool  stainlessHint = false;
  bool  useF = true;        // display unit (persisted, base spec §4)
  bool  muted = false;
  uint8_t brightnessLevel = 2;  // 0=Low,1=Medium,2=High (Settings, persisted)

  // NTP clock (Settings). timeValid=false until SNTP syncs (or no Wi-Fi).
  uint8_t tzIndex = 0;      // index into TIMEZONES
  bool    timeValid = false;
  uint8_t clockHour = 0, clockMin = 0;

  // Target Assist (M3) + presets (M4)
  GuidanceState guidance = GuidanceState::IDLE;
  int   targetCenterF = 350;
  int   etaSeconds = -1;    // -1 = estimating/unknown
  float projectedPeakF = 0;
  uint8_t presetId = PRESET_GENERIC;

  // Recovery Monitor (M6)
  bool recovering = false;
  RecoveryHint recoveryHint = RecoveryHint::NONE;
  bool addBatchPrompt = false;   // brief "add next batch" after recovery

  // Learn Pan Mode (M6)
  LearnPhase learnPhase = LearnPhase::OFF;
  uint8_t learnProgress = 0;     // 0..100 while RECORDING
  float learnedLagMinutes = 0;

  // Food timer (M12.5)
  const FoodEntry* food = nullptr;   // selected food (nullptr = plain target)
  FoodTimerOut foodTimer;            // phase/side/remaining/k
  uint8_t batchCount = 0;            // completed batches this session
  bool foodCue = false;             // FLIP/REMOVE cue pending
  const char* foodCueVerb = "";
  const char* foodCueSub = "";

  // Post-cook feedback prompt (spec §2.7): after REMOVE, ask Under/Perfect/Over.
  bool feedbackPrompt = false;
  const char* feedbackName = "";    // food being graded

  // Battery / power (M7)
  BatteryState battery;
  bool pluginWarning = false;    // critical battery mid-cook -> "PLUG ME IN"

  // Multi-pan zone 2 (M12)
  bool zone2Present = false;
  float zone2TempC = 0;
  GuidanceState zone2Guidance = GuidanceState::IDLE;
  int zone2TargetF = 300;

  // Per-zone food timer (Phase 3): zone 2 runs its own cook independently.
  const FoodEntry* zone2Food = nullptr;
  FoodTimerOut zone2FoodTimer;
  uint8_t zone2Batch = 0;

  // Recipe sequencer (M19)
  bool recipeActive = false;
  const char* recipeCue = "";
  int recipeStepIndex = 0;

  // Autopilot / ASSIST control (M14-M18, M21). armed=false => pure ADVISORY.
  bool assistArmed = false;
  bool actuatorAvailable = false; // an actuator is configured (arming allowed)
  float assistDuty = 0;          // 0..1 commanded to the actuator
  int  assistInterlock = 0;      // Interlock enum (0 = NONE)
  const char* actuatorName = "";

  // PID autotune wizard (roadmap §3.2). 0=idle, 1=running, 2=done.
  uint8_t autotuneState = 0;
  uint8_t autotuneProgress = 0;  // observed oscillation cycles (0..5)
  float autotuneKp = 0, autotuneKi = 0, autotuneKd = 0;
};
