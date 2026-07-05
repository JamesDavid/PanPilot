// ota.h — OTA boot-loop rollback guard (roadmap spec §2.2, M10). Device-only.
// Counts boots in NVS; if a freshly-OTA'd image boot-loops 3 times it rolls back
// to the previous slot. Once the app runs stably for a few seconds we mark it
// valid and clear the counter. Compile-verified; rollback is bench-tested by
// flashing a deliberately boot-looping image (HARDWARE_TEST M10).
#pragma once
#if !defined(PANPILOT_SIM)

namespace hal {
void ota_boot_guard();     // call very early in setup(): may roll back + reboot
void ota_mark_stable();    // call once after a few seconds of healthy runtime
}  // namespace hal
#endif
