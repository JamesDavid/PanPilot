// foodtimer.cpp — see foodtimer.h.
#include "foodtimer.h"
#include <algorithm>

float FoodTimer::compFactor(const FoodEntry* e, float panTempF) {
  if (!e) return 1.0f;
  const float k = 1.0f + (e->tempCompPctPer25F / 100.0f) *
                             ((e->refTempF - panTempF) / 25.0f);
  return std::max(0.7f, std::min(1.3f, k));
}

void FoodTimer::start(const FoodEntry* e, uint32_t now, float timeFactor) {
  e_ = e; last_ = now; progress_ = 0; side_ = 0;
  timeFactor_ = timeFactor > 0.0f ? timeFactor : 1.0f;
  resting_ = false; restProg_ = 0; done_ = false;
}
void FoodTimer::stop() { e_ = nullptr; done_ = false; }

FoodTimerOut FoodTimer::update(float panTempF, uint32_t now) {
  FoodTimerOut o;
  if (!e_) { o.phase = FoodTimerOut::IDLE; return o; }
  if (done_) { o.phase = FoodTimerOut::DONE; return o; }

  // Guard the unsigned subtraction (now < last_ would underflow to a huge dt).
  float dt = (now >= last_) ? (now - last_) / 1000.0f : 0.0f;
  last_ = now;

  if (resting_) {
    restProg_ += dt;
    o.phase = FoodTimerOut::RESTING;
    o.remainingSec = std::max(0, (int)(e_->restSec - restProg_));
    if (restProg_ >= e_->restSec) {
      resting_ = false; done_ = true;
      o.phase = FoodTimerOut::DONE; o.event = FoodTimerOut::REST_DONE;
    }
    return o;
  }

  const float k = compFactor(e_, panTempF);
  progress_ += dt * k;
  o.k = k;
  o.phase = FoodTimerOut::COOKING;
  o.side = side_ + 1;
  const float target = e_->sideSec[side_] * timeFactor_;   // personalized (§2.7)
  o.remainingSec =
      std::max(0, (int)((target - progress_) / std::max(k, 0.1f)));

  if (progress_ >= target) {
    if (side_ + 1 < e_->sides) {                 // more sides -> FLIP/turn
      o.event = FoodTimerOut::FLIP;
      ++side_; progress_ = 0;
      o.side = side_ + 1;
    } else {                                     // last side done -> REMOVE
      o.event = FoodTimerOut::REMOVE;
      if (e_->restSec > 0) { resting_ = true; restProg_ = 0; }
      else done_ = true;
    }
  }
  return o;
}
