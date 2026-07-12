// serial_tee_redirect.h — force-included into every firmware translation unit
// (platformio.ini build_flags: -include serial_tee_redirect.h). Redirects
// `Serial` to the PanPilotTee tee (real UART + the /log ring, hal/weblog.h) so
// the whole firmware becomes Wi-Fi-debuggable without touching call sites.
// No-op for: C files, native tests (no ARDUINO), the simulator, and the tee's
// own implementation (PANPILOT_TEE_IMPL references the real Serial).
#pragma once
#if defined(__cplusplus) && defined(ARDUINO) && !defined(PANPILOT_SIM) && \
    !defined(PANPILOT_TEE_IMPL)
#include "hal/weblog.h"
#define Serial PanPilotTee
#endif
