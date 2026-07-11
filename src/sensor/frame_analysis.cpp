// frame_analysis.cpp — see frame_analysis.h. Fixed-size stack buffers only; no
// dynamic allocation in the per-frame path (base spec §13).
#include "frame_analysis.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr int R = THERM_ROWS, C = THERM_COLS, N = THERM_PIXELS;
inline int idx(int r, int c) { return r * C + c; }

float median_of(const float* a, int n) {
  static float tmp[N];
  for (int i = 0; i < n; ++i) tmp[i] = a[i];
  std::nth_element(tmp, tmp + n / 2, tmp + n);
  return tmp[n / 2];
}

// Label the 4-connected component containing `seed` into `label` with `id`,
// counting its size. Iterative flood fill over an explicit stack.
int flood(const bool* cand, int* label, int seed, int id) {
  static int stack[N];
  int sp = 0, size = 0;
  stack[sp++] = seed;
  label[seed] = id;
  while (sp) {
    const int p = stack[--sp];
    ++size;
    const int r = p / C, c = p % C;
    const int nb[4][2] = {{r - 1, c}, {r + 1, c}, {r, c - 1}, {r, c + 1}};
    for (auto& n : nb) {
      if (n[0] < 0 || n[0] >= R || n[1] < 0 || n[1] >= C) continue;
      const int q = idx(n[0], n[1]);
      if (cand[q] && label[q] < 0) { label[q] = id; stack[sp++] = q; }
    }
  }
  return size;
}

float percentile(float* vals, int n, int pct) {
  if (n <= 0) return 0.0f;
  int k = (pct * (n - 1)) / 100;
  std::nth_element(vals, vals + k, vals + n);
  return vals[k];
}
}  // namespace

void FrameAnalyzer::reset() {
  have_prev_ = false; ever_present_ = false; last_present_ms_ = 0;
  prev_hot_ = false; tracked_ = false; locked_ = false;
}
void FrameAnalyzer::lockRoi(float cx, float cy) {
  locked_ = true; lock_cx_ = cx; lock_cy_ = cy;
}
void FrameAnalyzer::clearLock() { locked_ = false; }

PanReading FrameAnalyzer::process(const ThermalFrame& f) {
  PanReading r{};
  r.t_ms = f.t_ms;
  r.presence = PanPresence::UNCERTAIN;
  r.backgroundC = f.ambientC;
  if (!f.valid) return r;

  const float* px = &f.px[0][0];
  const float bg = median_of(px, N);
  r.backgroundC = bg;

  // Candidate mask (§6.1): hot vs background, or absolute-hot while cooking.
  static bool cand[N];
  for (int i = 0; i < N; ++i)
    cand[i] = (px[i] > bg + cfg_.panDeltaC) || (px[i] > cfg_.panAbsHotC);

  auto pick_largest = [&](int* label) -> int {  // returns best label id or -1
    for (int i = 0; i < N; ++i) label[i] = -1;
    int id = 0, bestId = -1, bestSize = 0;
    for (int i = 0; i < N; ++i) {
      if (cand[i] && label[i] < 0) {
        const int sz = flood(cand, label, i, id);
        if (sz > bestSize) { bestSize = sz; bestId = id; }
        ++id;
      }
    }
    // Tap-to-lock (§6.3): if the user pinned an ROI, prefer the blob under the
    // lock point (disambiguates which pan on a two-burner scene) over the
    // largest; fall back to largest if the lock lands off any blob. The locked
    // blob's size is counted on demand (no per-blob array, to spare DRAM).
    if (locked_) {
      const int lr = (int)(lock_cy_ + 0.5f), lc = (int)(lock_cx_ + 0.5f);
      if (lr >= 0 && lr < R && lc >= 0 && lc < C) {
        const int ll = label[idx(lr, lc)];
        if (ll >= 0) {
          int sz = 0;
          for (int i = 0; i < N; ++i) if (label[i] == ll) ++sz;
          if (sz >= cfg_.minPanPixels) return ll;
        }
      }
    }
    return (bestSize >= cfg_.minPanPixels) ? bestId : -1;
  };

  static int label[N];
  int blobId = pick_largest(label);

  // Obstruction (§6.4): a hand or cool utensil over a previously-hot pan strips
  // the hot pixels but leaves a warm, non-ambient, non-hot mass at the last
  // known position. Gated on tracked_ (which SURVIVES missed frames — a single
  // flicker of the cover below the warm window no longer disables detection
  // for the rest of the obstruction, as gating on the smoothing state did).
  // Checked before the cooling fallback, which would otherwise grab the warm
  // cover and mislabel it a pan.
  if (blobId < 0 && tracked_ && prev_hot_) {
    const int cr = (int)(track_cy_ + 0.5f), cc0 = (int)(track_cx_ + 0.5f);
    int warm = 0, total = 0;
    for (int dr = -2; dr <= 2; ++dr)
      for (int dc = -2; dc <= 2; ++dc) {
        const int rr = cr + dr, cc = cc0 + dc;
        if (rr < 0 || rr >= R || cc < 0 || cc >= C) continue;
        ++total;
        const float v = f.px[rr][cc];
        if (v > f.ambientC + OBSTRUCT_WARM_DELTA_C && v < OBSTRUCT_COVER_MAX_C) ++warm;
      }
    if (total > 0 && warm >= (int)(total * OBSTRUCT_COVER_FRAC)) {
      r.presence = PanPresence::OBSTRUCTED;
      r.roiCx = track_cx_; r.roiCy = track_cy_;  // hold the last known location
      r.confidence = 0;
      // The pan is still believed there — refresh the presence clock so a long
      // obstruction can't snap straight to ABSENT on one below-window frame.
      last_present_ms_ = f.t_ms;
      return r;                                  // keep tracked_/prev_hot_
    }
  }

  // Cooling-pan fallback (§6.1): a cooling pan can drop below the delta; if we
  // were tracking one, re-threshold at ambient+cooling NEAR THE LAST POSITION.
  // The distance gate is what makes "near" true — without it any residual
  // warmth anywhere in view (burner ring, a hand) became "the pan" and ABSENT
  // never fired after a real removal.
  if (blobId < 0 && tracked_) {
    for (int i = 0; i < N; ++i)
      cand[i] = px[i] > f.ambientC + cfg_.coolingAbsDelta;
    blobId = pick_largest(label);
    if (blobId >= 0) {
      int cnt = 0; float sx = 0, sy = 0;
      for (int rr = 0; rr < R; ++rr)
        for (int cc = 0; cc < C; ++cc)
          if (label[idx(rr, cc)] == blobId) { ++cnt; sx += cc; sy += rr; }
      const float d = std::hypot(sx / cnt - track_cx_, sy / cnt - track_cy_);
      if (d > 8.0f) blobId = -1;   // warm thing elsewhere is not our pan
    }
  }

  if (blobId < 0) {
    // No pan. ABSENT only after the hysteresis window (§6.3).
    const bool absent = !ever_present_ ||
                        (f.t_ms - last_present_ms_) > cfg_.absentMs;
    r.presence = absent ? PanPresence::ABSENT : PanPresence::UNCERTAIN;
    if (absent) { tracked_ = false; prev_hot_ = false; }  // position expires
    have_prev_ = false;
    return r;
  }

  // Blob geometry + centroid.
  int count = 0; float sx = 0, sy = 0;
  for (int rr = 0; rr < R; ++rr)
    for (int cc = 0; cc < C; ++cc)
      if (label[idx(rr, cc)] == blobId) { ++count; sx += cc; sy += rr; }
  float cx = sx / count, cy = sy / count;

  // Interior = erode by one pixel (drop blob px with a non-blob 4-neighbor).
  static float interior[N]; int nin = 0;
  static float blobTemps[N]; int nblob = 0;
  auto inBlob = [&](int rr, int cc) {
    return rr >= 0 && rr < R && cc >= 0 && cc < C &&
           label[idx(rr, cc)] == blobId;
  };
  for (int rr = 0; rr < R; ++rr)
    for (int cc = 0; cc < C; ++cc) {
      if (label[idx(rr, cc)] != blobId) continue;
      blobTemps[nblob++] = f.px[rr][cc];
      if (inBlob(rr - 1, cc) && inBlob(rr + 1, cc) && inBlob(rr, cc - 1) &&
          inBlob(rr, cc + 1))
        interior[nin++] = f.px[rr][cc];
    }
  float* src = (nin > 0) ? interior : blobTemps;
  int nsrc = (nin > 0) ? nin : nblob;

  // panTemp = 75th percentile of interior (§6.2); min/max for the UI hotspot.
  float pmin = src[0], pmax = src[0];
  for (int i = 1; i < nsrc; ++i) { pmin = std::min(pmin, src[i]); pmax = std::max(pmax, src[i]); }
  r.panTempC = percentile(src, nsrc, cfg_.roiPercentile);
  r.roiMinC = pmin; r.roiMaxC = pmax;
  r.roiPixelCount = static_cast<uint16_t>(count);

  // Centroid low-pass (anti-jitter, §6.3) + movement for confidence.
  float jump = 0;
  if (have_prev_) {
    jump = std::sqrt((cx - prev_cx_) * (cx - prev_cx_) +
                     (cy - prev_cy_) * (cy - prev_cy_));
    cx = cfg_.centroidAlpha * cx + (1 - cfg_.centroidAlpha) * prev_cx_;
    cy = cfg_.centroidAlpha * cy + (1 - cfg_.centroidAlpha) * prev_cy_;
  }
  prev_cx_ = cx; prev_cy_ = cy; have_prev_ = true;
  r.roiCx = cx; r.roiCy = cy;
  r.moved = (jump > MOVED_JUMP_PX);   // fast centroid jump -> CHECK AIM (§6.4)

  // Confidence (§6.3): blob area adequacy + tight interior spread + stability.
  const float spread = pmax - pmin;
  const float areaScore = std::min(1.0f, count / 60.0f);
  const float spreadScore = std::max(0.0f, 1.0f - spread / 50.0f);
  const float stabScore = std::max(0.0f, 1.0f - jump / 6.0f);
  int conf = static_cast<int>(
      100.0f * (0.4f * areaScore + 0.3f * spreadScore + 0.3f * stabScore));

  // Reflective-stainless signature (§6.3/§7.5): high spatial variance + an
  // implausibly low reading -> cap confidence, raise the hint.
  if (spread > cfg_.stainlessSpread && r.panTempC < 150.0f) {
    r.stainlessHint = true;
    conf = std::min(conf, cfg_.stainlessCap);
  }
  r.confidence = static_cast<uint8_t>(std::max(0, std::min(100, conf)));

  ever_present_ = true;
  last_present_ms_ = f.t_ms;
  prev_hot_ = r.panTempC > OBSTRUCT_WAS_HOT_C;   // gate for obstruction detection
  tracked_ = true;                               // raw (unsmoothed) position —
  track_cx_ = sx / count;                        // survives missed frames until
  track_cy_ = sy / count;                        // ABSENT finally declares
  r.presence = (r.confidence < cfg_.uncertainConf) ? PanPresence::UNCERTAIN
                                                   : PanPresence::PRESENT;
  return r;
}
