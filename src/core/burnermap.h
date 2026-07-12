// burnermap.h — per-pan burner calibration (Phase 3 "burner suggestions").
// For a given pan on a given burner, each knob setting produces a measurable
// heating rate; one burner-off window gives the loss coefficient. With the
// linear plant model  dT/dt = P(s) - kLoss*(T - ambient)  that predicts the
// SETTLE temperature of every knob setting, so down-cues can say "aim knob at
// MED-LOW" from THIS pan's data instead of a generic table.
//
// Hardware-free + unit-tested against the same first-order plant as the PID
// tests. The wizard drives BurnerMapper; the solved BurnerMap is stored on the
// PanProfile.
#pragma once
#include <stdint.h>

#define BURNER_SETTINGS 5
const char* burner_setting_name(int i);   // "LOW".."HIGH"

struct BurnerMap {
  float rateF[BURNER_SETTINGS];      // heating rate °F/min at each setting
  float measTempF[BURNER_SETTINGS];  // pan temp where that rate was measured
  float coolRateF;                   // burner-off rate °F/min (negative)
  float coolTempF;                   // pan temp where cooling was measured
  float ambientF;
  bool  valid;
};

// Loss coefficient (per minute). <= 0 means the map is unusable.
float burnermap_kloss(const BurnerMap& m);
// Predicted hold temperature for a knob setting (°F). 0 if invalid.
float burnermap_settleF(const BurnerMap& m, int setting);
// The setting whose predicted settle temp is nearest the target. -1 invalid.
int burnermap_suggest(const BurnerMap& m, float targetF);
// Convenience: the suggested setting's name, or nullptr if the map is unusable.
const char* burnermap_suggest_name(const BurnerMap& m, float targetF);

// Guided measurement state machine. The wizard prompts the user to set the
// knob, confirm(), then SETTLE (transient dies) and MEASURE (endpoint slope)
// windows run on the frame feed; after HIGH comes one burner-OFF cooling
// measurement. Times are short enough for a ~5-minute wizard.
class BurnerMapper {
 public:
  enum Phase : uint8_t { IDLE, AWAIT_KNOB, SETTLING, MEASURING,
                         AWAIT_OFF, COOL_SETTLING, COOL_MEASURING, DONE };
  static constexpr uint32_t SETTLE_MS = 15000;
  static constexpr uint32_t MEASURE_MS = 30000;
  static constexpr uint32_t COOL_SETTLE_MS = 10000;
  static constexpr uint32_t COOL_MEASURE_MS = 30000;

  void start(float ambientF, uint32_t now);
  void confirm(uint32_t now);   // user confirmed the prompted knob position
  void cancel();
  void update(float tempF, uint32_t now);

  Phase phase() const { return phase_; }
  int setting() const { return idx_; }    // setting being prompted/measured
  int secondsLeft(uint32_t now) const;    // in a settle/measure window
  const BurnerMap& result() const { return map_; }

 private:
  Phase phase_ = IDLE;
  int idx_ = 0;
  uint32_t t0_ = 0;
  float startT_ = 0;
  BurnerMap map_ = {};

  void finishIfComplete();
};
