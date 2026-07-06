// multipan.cpp — see multipan.h.
#include "multipan.h"
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
int flood(const bool* cand, int* label, int seed, int id) {
  static int stack[N];
  int sp = 0, size = 0;
  stack[sp++] = seed; label[seed] = id;
  while (sp) {
    const int p = stack[--sp]; ++size;
    const int r = p / C, c = p % C;
    const int nb[4][2] = {{r-1,c},{r+1,c},{r,c-1},{r,c+1}};
    for (auto& n : nb) {
      if (n[0] < 0 || n[0] >= R || n[1] < 0 || n[1] >= C) continue;
      const int q = idx(n[0], n[1]);
      if (cand[q] && label[q] < 0) { label[q] = id; stack[sp++] = q; }
    }
  }
  return size;
}
float percentile(float* v, int n, int pct) {
  if (n <= 0) return 0;
  int k = (pct * (n - 1)) / 100;
  std::nth_element(v, v + k, v + n);
  return v[k];
}

// Build a PanReading for the pixels labelled `blobId`.
PanReading readBlob(const ThermalFrame& f, const int* label, int blobId,
                    float bg) {
  PanReading r{};
  int count = 0; float sx = 0, sy = 0;
  static float interior[N], blobT[N]; int nin = 0, nbl = 0;
  auto inB = [&](int rr, int cc) {
    return rr >= 0 && rr < R && cc >= 0 && cc < C && label[idx(rr, cc)] == blobId;
  };
  for (int rr = 0; rr < R; ++rr)
    for (int cc = 0; cc < C; ++cc) {
      if (label[idx(rr, cc)] != blobId) continue;
      ++count; sx += cc; sy += rr;
      blobT[nbl++] = f.px[rr][cc];
      if (inB(rr-1,cc) && inB(rr+1,cc) && inB(rr,cc-1) && inB(rr,cc+1))
        interior[nin++] = f.px[rr][cc];
    }
  float* src = nin ? interior : blobT; int ns = nin ? nin : nbl;
  float pmin = src[0], pmax = src[0];
  for (int i = 1; i < ns; ++i) { pmin = std::min(pmin, src[i]); pmax = std::max(pmax, src[i]); }
  r.panTempC = percentile(src, ns, ROI_PERCENTILE);
  r.roiMinC = pmin; r.roiMaxC = pmax;
  r.roiPixelCount = (uint16_t)count;
  r.roiCx = sx / count; r.roiCy = sy / count;
  r.backgroundC = bg;
  const float spread = pmax - pmin;
  int conf = (int)(100.0f * (0.5f * std::min(1.0f, count / 60.0f) +
                             0.5f * std::max(0.0f, 1.0f - spread / 50.0f)));
  r.confidence = (uint8_t)std::max(0, std::min(100, conf));
  r.presence = r.confidence < CONFIDENCE_UNCERTAIN ? PanPresence::UNCERTAIN
                                                   : PanPresence::PRESENT;
  return r;
}
}  // namespace

void MultiPanTracker::reset() { have_[0] = have_[1] = false; }

int MultiPanTracker::process(const ThermalFrame& f, PanReading out[2]) {
  out[0] = PanReading{}; out[1] = PanReading{};
  out[0].presence = out[1].presence = PanPresence::ABSENT;
  if (!f.valid) return 0;

  const float* px = &f.px[0][0];
  const float bg = median_of(px, N);
  static bool cand[N];
  for (int i = 0; i < N; ++i)
    cand[i] = (px[i] > bg + PAN_DELTA_C) || (px[i] > PAN_ABS_HOT_C);

  static int label[N];
  for (int i = 0; i < N; ++i) label[i] = -1;
  // Find the two largest components.
  int id = 0, best[2] = {-1, -1}, bestSz[2] = {0, 0};
  for (int i = 0; i < N; ++i) {
    if (cand[i] && label[i] < 0) {
      int sz = flood(cand, label, i, id);
      if (sz > bestSz[0]) { bestSz[1] = bestSz[0]; best[1] = best[0]; bestSz[0] = sz; best[0] = id; }
      else if (sz > bestSz[1]) { bestSz[1] = sz; best[1] = id; }
      ++id;
    }
  }

  PanReading found[2]; int nf = 0;
  for (int b = 0; b < 2; ++b)
    if (best[b] >= 0 && bestSz[b] >= MIN_PAN_PIXELS)
      found[nf++] = readBlob(f, label, best[b], bg);

  // Associate found blobs to persistent zones by nearest previous centroid.
  bool usedZone[2] = {false, false};
  bool assigned[2] = {false, false};
  for (int i = 0; i < nf; ++i) {
    int bestZ = -1; float bestD = 1e9f;
    for (int z = 0; z < 2; ++z) {
      if (!have_[z] || usedZone[z]) continue;
      float d = std::hypot(found[i].roiCx - cx_[z], found[i].roiCy - cy_[z]);
      if (d < bestD) { bestD = d; bestZ = z; }
    }
    if (bestZ >= 0 && bestD < 8.0f) {
      out[bestZ] = found[i]; usedZone[bestZ] = true; assigned[i] = true;
      cx_[bestZ] = found[i].roiCx; cy_[bestZ] = found[i].roiCy;
    }
  }
  // Place any unassigned blobs into free zones (new pans).
  for (int i = 0; i < nf; ++i) {
    if (assigned[i]) continue;
    for (int z = 0; z < 2; ++z)
      if (!usedZone[z]) {
        out[z] = found[i]; usedZone[z] = true; have_[z] = true;
        cx_[z] = found[i].roiCx; cy_[z] = found[i].roiCy;
        break;
      }
  }
  for (int z = 0; z < 2; ++z) if (!usedZone[z]) have_[z] = false;
  return nf;
}
