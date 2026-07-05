// screen_lastcook.h — "Last Cook" summary: a trace sparkline + key stats
// (roadmap spec §2.3). Portable LVGL; owns its screen.
#pragma once
#include <lvgl.h>
#include "core/session.h"

namespace ui {
lv_obj_t* lastcook_create();
// trace is tempC*10 at 1 Hz, n samples. hasData=false shows an empty state.
void lastcook_update(const SessionSummary& s, const int16_t* traceC10, int n,
                     bool hasData, bool useF);
}  // namespace ui
