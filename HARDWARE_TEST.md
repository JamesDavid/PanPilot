# PanPilot — Hardware Test Checklist

Per-milestone bench procedures for anything the CI/simulator can't observe. A
milestone stays **pending hardware verification** until James checks these off on
the physical device. Expected results are stated so a failure is unambiguous.

Board under test: **CrowPanel Advance 3.5″ (ESP32-S3-WROOM-1-N16R8)**.

---

## M0 — Board bring-up

Flash: web flasher (https://jamesdavid.github.io/PanPilot/) → "CrowPanel Advance
3.5″ (ESP32-S3)", **or** `pio run -e crowpanel35_advance -t upload`.

| # | Step | Expected | ✅ |
|---|---|---|---|
| 0.1 | Power via USB-C, open serial @115200 | Boot banner: `[PanPilot] CrowPanel Advance 3.5" (ESP32-S3)  fw=<ver>` then `M0 ready — tap BEEP` | ☐ |
| 0.2 | Observe the screen | Dark screen, white **“PanPilot”** title, version subtitle, green **BEEP** button centered | ☐ |
| 0.3 | Backlight | Screen is lit at full brightness (not dim/black) | ☐ |
| 0.4 | Boot sound | A single short **chirp** at startup | ☐ |
| 0.5 | Touch the BEEP button | Button reacts (visual press) **and** a chirp sounds | ☐ |
| 0.6 | Touch accuracy | Press lands on the button where you touch (no large offset / inverted axis) | ☐ |
| 0.7 | Flasher | The Pages web flasher loads, detects the S3, and completes a flash | ☐ |

**If 0.5/0.6 fail (touch):** note whether touch does nothing (GT911 not found —
check `TOUCH_RST`: direct GPIO48 vs PCA9557 TP_RST line) or is offset/mirrored
(axis mapping / rotation — report which corner a top-left press registers at, and
I'll fix `TOUCH_MAP_*` / rotation in `board_pins.h` / the LGFX config).

**Bench-confirm items flagged during pin verification (report actual values):**
- `TFT_BL` (GPIO38) polarity — is full-on HIGH or LOW? Any flicker at PWM 5 kHz?
- `BUZZER_PIN` (GPIO8) — does the onboard buzzer actually sound? Passive or active?
- Touch reset line — GPIO48 vs PCA9557 expander `TP_RST`.
- Battery ADC pin (undocumented) — for Phase 2 power subsystem.

---

## M1 — Thermal pipeline _(added when M1 lands)_
## M2 — Thermometer Mode _(added when M2 lands)_

_(Later milestones append their sections here.)_
