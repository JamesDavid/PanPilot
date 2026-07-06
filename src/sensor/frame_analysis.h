// frame_analysis.h — turn a ThermalFrame into a PanReading (base spec §6).
// Replaces the laser + ToF: pan blob detection, interior ROI temperature,
// confidence, presence classification. Pure C++ (no Arduino/LVGL) so it is
// unit-tested natively (base spec §3, §11). Stateful across frames (centroid
// smoothing, presence hysteresis) — one instance per tracked pan.
#pragma once
#include "pan_types.h"
#include "app_config.h"

class FrameAnalyzer {
 public:
  struct Config {
    float panDeltaC       = PAN_DELTA_C;
    float panAbsHotC      = PAN_ABS_HOT_C;
    int   minPanPixels    = MIN_PAN_PIXELS;
    float coolingAbsDelta = COOLING_ABS_DELTA_C;
    int   roiPercentile   = ROI_PERCENTILE;
    float centroidAlpha   = CENTROID_LP_ALPHA;
    uint32_t absentMs     = PRESENCE_ABSENT_MS;
    int   uncertainConf   = CONFIDENCE_UNCERTAIN;
    float stainlessSpread = STAINLESS_SPREAD_C;
    int   stainlessCap    = STAINLESS_CONF_CAP;
  };

  FrameAnalyzer() = default;
  explicit FrameAnalyzer(const Config& c) : cfg_(c) {}

  // Analyze one frame. Invalid frames yield presence=UNCERTAIN, confidence=0.
  PanReading process(const ThermalFrame& f);

  void reset();

  // Lock a circular ROI at a pixel location (thermal-view tap-to-lock, §6.3).
  void lockRoi(float cx, float cy);
  void clearLock();          // back to auto-follow
  bool locked() const { return locked_; }

 private:
  Config cfg_;
  bool     have_prev_ = false;
  float    prev_cx_ = 0, prev_cy_ = 0;
  uint32_t last_present_ms_ = 0;
  bool     ever_present_ = false;
  bool     prev_hot_ = false;    // last tracked pan was hot (obstruction gate)
  bool     locked_ = false;
  float    lock_cx_ = 0, lock_cy_ = 0;
};
