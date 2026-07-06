// compliance.cpp — see compliance.h.
#include "compliance.h"

void ComplianceChecker::start(bool expectFall, uint32_t now) {
  expectFall_ = expectFall;
  start_ms_ = now;
  state_ = Compliance::PENDING;
}

Compliance ComplianceChecker::update(float rate, uint32_t now) {
  if (state_ != Compliance::PENDING) return state_;
  const bool responded =
      expectFall_ ? (rate < COMPLY_RATE_FMIN) : (rate > COMPLY_RATE_FMIN);
  if (responded) state_ = Compliance::COMPLIED;
  else if (now - start_ms_ > COMPLY_WINDOW_MS) state_ = Compliance::FAILED;
  return state_;
}
