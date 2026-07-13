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

## M1 — Thermal pipeline

Flash `crowpanel35_advance`. **Wire the MLX90640 first** (see wiring note below).

> ⚠️ **#1 M1 bench item — shared I²C bus.** The MLX90640 shares SDA=15 / SCL=16
> with the GT911 touch. Firmware puts touch on I²C port 0 (LovyanGFX) and the
> MLX on port 1 (Wire1), same pins, serialized by a mutex. **Verify touch AND
> sensor both work simultaneously.** If they interfere (touch dead while reading
> frames, or sensor NACKs), the fix is to move the MLX to dedicated GPIO_D pins —
> report which fails and I'll switch the bus config in `board_pins.h`.

| # | Step | Expected | ✅ |
|---|---|---|---|
| 1.1 | Boot with sensor wired, watch serial | `MLX90640 online`, then `[reading] ...` lines at ~4 Hz | ☐ |
| 1.2 | Sensor missing/unplugged | No crash; retries every 2 s; no fake readings | ☐ |
| 1.3 | Point at a hot mug/pan | Blob appears in the thermal view, tracks as you move the head | ☐ |
| 1.4 | **Accuracy** — cast-iron pan vs a reference IR thermometer | `panTemp` within **±5 °C** of reference (base spec M1 accept) | ☐ |
| 1.5 | Aim hint | “Center the pan” → “Good aim” as the blob nears the crosshair | ☐ |
| 1.6 | Tap-to-lock | Tapping the pan locks the ROI; Auto returns to follow | ☐ |
| 1.7 | Touch + sensor together | Touch still responds while frames stream (shared-bus check) | ☐ |
| 1.8 | Bare stainless pan | Reads low + `STAINLESS?` in serial; confidence capped | ☐ |

**Wiring (MLX90640 → CrowPanel Advance I²C header):** VIN→3V3, GND→GND,
SDA→GPIO15, SCL→GPIO16. Keep the lead ≤ 30 cm for 400 kHz–1 MHz margin.

## M2 — Thermometer Mode

Flash `crowpanel35_advance` (sensor wired).

| # | Step | Expected | ✅ |
|---|---|---|---|
| 2.1 | Heat a pan, watch the home screen | Big temperature rises **smoothly** (no jitter/jumping) | ☐ |
| 2.2 | Rate line | Shows *estimating…* first, then e.g. `▲ 40°F/min`; arrow matches reality | ☐ |
| 2.3 | State text | “Heating”/“Heating fast” while rising, “Stable” when steady, “Cooling” off-heat | ☐ |
| 2.4 | Remove the pan | Within ~3 s shows `--` and “No pan — aim the sensor” | ☐ |
| 2.5 | Jerk the sensor head | “Check aim” appears briefly | ☐ |
| 2.6 | Tap the temperature | Opens the thermal view; **Done** returns home | ☐ |
| 2.7 | Tap °F/°C | Units switch everywhere; **reboot** → the choice persists (NVS) | ☐ |
| 2.8 | Accuracy sanity | Steady-state reading tracks a reference thermometer within a few °C | ☐ |

## M3 — Target Assist

Flash `crowpanel35_advance`. **Acceptance (base spec M3):** preheat cast iron to
350 °F and confirm the `HOLD → TURN_DOWN_SOON → READY` sequence.

| # | Step | Expected | ✅ |
|---|---|---|---|
| 3.1 | Set target 350 °F with –/+ | Target updates in 5 °F steps; persists across reboot | ☐ |
| 3.2 | Heat from cold | Action bar: **Heat more/Hold** (blue) with **ETA** counting down | ☐ |
| 3.3 | Approaching target | **Turn down soon** (amber) within ~15 °F below the band | ☐ |
| 3.4 | Enter the band | **READY** full-screen (green) + single chime; fires once | ☐ |
| 3.5 | Leave & re-enter band (>10 °F) | READY chimes again (re-armed), not on small wobbles | ☐ |
| 3.6 | Overheat past warn (450 °F) | **TOO HOT** full-screen (red) + repeating alarm every ~10 s | ☐ |
| 3.7 | **Overshoot** — blast on HIGH | **TURN DOWN NOW** fires *before* reaching target when the projected peak exceeds it | ☐ |

_(A mute toggle + settings screen arrive with a later milestone; mute state is
persisted but not yet user-togglable on-device.)_

## M4 — Overshoot prediction + presets

Flash `crowpanel35_advance`. **Acceptance (base spec M4):** on a fast HIGH-burner
ramp, TURN DOWN NOW fires *before* target and the predicted peak is within
±25 °F of the actual peak.

| # | Step | Expected | ✅ |
|---|---|---|---|
| 4.1 | Tap the preset name → picker | Grid of 6 presets with bands; **Done** returns | ☐ |
| 4.2 | Pick **Sear** | Target becomes 475–550 °F; name shows “Sear”; persists across reboot | ☐ |
| 4.3 | Pick **Stainless** | Target 400–450 °F; home shows the *bare stainless reads low* banner | ☐ |
| 4.4 | **Overshoot on HIGH** | TURN DOWN NOW fires before target; the alert shows **peak ~XXX°F** | ☐ |
| 4.5 | Predicted-peak accuracy | The predicted peak is within **±25 °F** of the actual peak reached | ☐ |
| 4.6 | Nudge –/+ after a preset | Name reverts to “Generic” (custom); band shifts in 5 °F steps | ☐ |

## M5 — Auto-wake + sessions

Flash `crowpanel35_advance`. **Acceptance (base spec M5):** device dims after a
cook and wakes by itself when a burner is lit. (Idle timeout is 10 min — to test
faster, temporarily lower `IDLE_TIMEOUT_MS` in `app_config.h`.)

| # | Step | Expected | ✅ |
|---|---|---|---|
| 5.1 | Leave untouched, pan cool, 10 min | Screen dims to ~10% and shows **“PanPilot — monitoring”** | ☐ |
| 5.2 | Tap the screen while idle | Wakes to full brightness + home, single chirp | ☐ |
| 5.3 | From idle, put a pan on heat | **Wakes by itself** + chirp; serial: `heating detected — select a target` | ☐ |
| 5.4 | Stays awake while cooking | No dimming while the scene is hot, even with no touches | ☐ |
| 5.5 | Finish a cook, let it cool ~10 min | Serial: `[session] saved` (summary stored to the 10-slot NVS ring) | ☐ |

## M6 — Learn Pan Mode + Recovery Monitor (Phase 1.5)

Flash `crowpanel35_advance`. **Acceptance (base spec M6):** pancake batch test —
device announces “Add next batch” correctly for 3 consecutive batches.

| # | Step | Expected | ✅ |
|---|---|---|---|
| 6.1 | Pick **Pancakes**, preheat to READY | Normal Target Assist to the band | ☐ |
| 6.2 | Pour batter (food added) | Serial `[recovery] food added`; action bar → **Recovering** | ☐ |
| 6.3 | Pan climbs back into band | **ADD NEXT BATCH** full-screen (green) + chime; serial `pan recovered` | ☐ |
| 6.4 | Repeat 3 batches | Cue fires correctly all 3 times | ☐ |
| 6.5 | Slow recovery (low heat) | Note: *“Recovery slow — raise heat?”* | ☐ |
| 6.6 | Learn Pan → Start (empty pan, MEDIUM) | Progress bar fills over 30 s | ☐ |
| 6.7 | Learn Pan → Save | Shows learned lag; serial `[profile] loaded lag=…` after reboot | ☐ |
| 6.8 | Overshoot after learning | TURN DOWN NOW timing reflects the learned lag (vs generic) | ☐ |

---
# PHASE 2

## M7 — Power & Battery

Requires the battery add-ons (roadmap §2.1): 1S LiPo, charge/protect board,
MAX17048 fuel gauge on the shared I²C bus, optional USB-detect GPIO. The firmware
degrades gracefully to “USB, unknown SoC” when no gauge is fitted.

| # | Step | Expected | ✅ |
|---|---|---|---|
| 7.1 | Fit the MAX17048, boot | Home status bar shows a battery % (or ⚡ on USB) | ☐ |
| 7.2 | Unplug mid-cook | Device keeps guiding; SoC reads plausibly; backlight dims to ~70% | ☐ |
| 7.3 | Discharge to ≤15% | Serial `[batt] low` | ☐ |
| 7.4 | Discharge to ≤5% | Full-screen red **“PLUG ME IN”** + urgent alarm; serial `CRITICAL` | ☐ |
| 7.5 | Plug back in | Warning clears; battery shows ⚡ | ☐ |

**Deferred to the battery-hardware bring-up (need the LiPo path to develop +
measure):** DEEP IDLE light-sleep, SHIP MODE power-off, `SENSOR_PWR_EN` sensor
power-gating, and the **≤3 mA deep-idle standby** measurement (roadmap §2.1).
These are intentionally not yet implemented so they can be built and measured
against real hardware rather than blind.

## M8 — Wi-Fi + web dashboard

Advance env only (Wi-Fi is compiled in for the S3; the classic-ESP32 basic board
omits it for flash reasons). **Compile-verified; behavior is bench-tested here.**

| # | Step | Expected | ✅ |
|---|---|---|---|
| 8.1 | First boot, unprovisioned | A **PanPilot-XXXX** Wi-Fi hotspot appears (captive portal) | ☐ |
| 8.2 | Join it, enter home Wi-Fi | Device connects; serial prints the URL | ☐ |
| 8.3 | Browse **http://panpilot.local/** | Dashboard loads (temp, rate, ETA, target, action bar) | ☐ |
| 8.4 | Heat a pan | Dashboard updates live (~2 Hz); action bar color matches the device | ☐ |
| 8.5 | Watch the browser thermal canvas | 32×24 thermal image renders + tracks the pan | ☐ |
| 8.6 | Turn Wi-Fi off / leave network | All on-device cooking features keep working (local-first) | ☐ |

_Acceptance (roadmap M8): live temp + thermal view in a phone browser via
`panpilot.local` during a real cook._

## M9 — MQTT + Home Assistant

Advance env. Enter an MQTT broker in the Wi-Fi setup portal (blank = MQTT off).
**Compile-verified; needs a broker + HA to exercise.**

| # | Step | Expected | ✅ |
|---|---|---|---|
| 9.1 | Set broker, reboot | Serial `[ha] MQTT connected, discovery published` | ☐ |
| 9.2 | Open Home Assistant | A **PanPilot** device appears with temp/rate/guidance/pan sensors | ☐ |
| 9.3 | Controls | Mute (switch), Target (number), Preset (select) work from HA | ☐ |
| 9.4 | Availability | Powering PanPilot off shows the entities *unavailable* (LWT) | ☐ |
| 9.5 | Automation | An HA automation on guidance = “Too hot” (e.g. flash lights) fires | ☐ |

## M10 — OTA + dual partition

Advance env (dual-OTA partition `partitions_advance.csv`). **Compile-verified;
rollback must be exercised by flashing a bad image.**

| # | Step | Expected | ✅ |
|---|---|---|---|
| 10.1 | Browse **/update**, upload a good `firmware.bin` | Uploads, reboots into the new image; runs normally | ☐ |
| 10.2 | Confirm slot swap | `pio run -t upload` again / serial shows it booted the other OTA slot | ☐ |
| 10.3 | **Rollback** — upload a deliberately boot-looping image | After 3 failed boots, serial `boot-loop detected — rolling back`; device reverts to the previous working image | ☐ |
| 10.4 | Healthy boot | Serial `new image marked valid` ~8 s after a good boot | ☐ |

_Acceptance (roadmap M10): flash a deliberately boot-looping image; device
self-reverts._

## M11 — LittleFS sessions + web history

Advance env (LittleFS). **Acceptance (roadmap M11): 5 cooks logged, traces
render, CSV opens in a spreadsheet.**

| # | Step | Expected | ✅ |
|---|---|---|---|
| 11.1 | Do 5 short cooks (let each cool) | Serial `[session] saved to LittleFS` each time | ☐ |
| 11.2 | Open **Last Cook** (preset picker) | Trace sparkline + stats for the latest cook | ☐ |
| 11.3 | Browse **/api/sessions** | JSON list of the 5 sessions | ☐ |
| 11.4 | **/api/session?id=N** | Downloads a CSV that opens cleanly in a spreadsheet | ☐ |
| 11.5 | Log > 20 cooks | Oldest are evicted (count-capped) | ☐ |

## M13 — Attention & cue escalation

Advance/basic. **Acceptance (roadmap M13): L0–L3 demonstrated; Advisor compliance
verification works on a live burner.**

| # | Step | Expected | ✅ |
|---|---|---|---|
| 13.1 | Reach READY | L1: single chirp, bar pulse | ☐ |
| 13.2 | Overshoot band (not warn) | L2: full-screen card + backlight strobe + double-beep every 5 s | ☐ |
| 13.3 | Exceed warn / 650 °F | L3: red + urgent alarm repeating until it clears | ☐ |
| 13.4 | Mute, then trigger L2 vs L3 | L2 silent (still strobes); **L3 still alarms** | ☐ |
| 13.5 | **Compliance** — on “TURN DOWN”, actually turn the knob down | Confirmation chirp; serial `knob turn confirmed by rate change` | ☐ |
| 13.6 | On “TURN DOWN”, ignore it | No confirm; if temp keeps rising it escalates to L3 (TOO HOT) | ☐ |
| 13.7 | Strobe safety | Backlight strobe is ≤ 3 Hz and brightness restores after the cue | ☐ |

## M12.5 — Food timer + cook-time database

**Acceptance (roadmap M12.5): pancake cook auto-starts on pour, FLIP fires at the
compensated time, batch counter tracks 3 batches; chicken shows the internal-temp
note on REMOVE; a cold pan visibly extends the countdown with the banner.**

| # | Step | Expected | ☐ |
|---|---|---|---|
| 12.5.1 | Preset picker → **Cook a food** → Pancakes | Target set to 350–375 °F; scrollable list works | ☐ |
| 12.5.2 | Pour batter | Timer auto-starts (FOOD ADDED); arc + “Side 1/2 — FLIP in m:ss” | ☐ |
| 12.5.3 | FLIP cue | At the compensated side-1 time: L2 card “FLIP” + flip hint + double-beep | ☐ |
| 12.5.4 | 3 batches | Batch counter advances 1→2→3; each auto-starts on pour | ☐ |
| 12.5.5 | **Cold pan** (−40 °F) | Countdown visibly longer; “Cooler pan — timer extended” banner | ☐ |
| 12.5.6 | Chicken breast → REMOVE | Shows **“verify 165 °F internal”**; note cannot be dismissed | ☐ |

## M12 — Multi-pan / multi-zone

**Acceptance (roadmap M12): two pans on front burners — simultaneous eggs
(300 °F) + sear preheat (500 °F) — correct independent READY alerts. Passes 3
consecutive runs.**

| # | Step | Expected | ☐ |
|---|---|---|---|
| 12.1 | Two pans on adjacent burners | Primary shows full-size; **Pan 2 tile** appears with its temp | ☐ |
| 12.2 | Tap the Pan 2 tile → pick Eggs | Zone-2 target = 300 °F, independent of the primary | ☐ |
| 12.3 | Primary = Sear (500 °F) | Two independent targets running at once | ☐ |
| 12.4 | Each reaches its band | **Independent READY** alerts (primary full-screen; zone 2 double-chirp + tile turns green) | ☐ |
| 12.5 | Move a pan | Each stays pinned to its zone (persistent IDs) — no swap | ☐ |
| 12.6 | Repeat 3× | Passes 3 consecutive runs | ☐ |

---
# TRACK B — Recipe programs

## M19 — Recipe sequencer engine (Advisor mode)

**Acceptance (roadmap M19): a smash-burger ×4 program runs end-to-end on the
stovetop; cues fire through the attention system; knob-cue compliance works
inside a program; a butter Prep step auto-advances on a thermally detected melt,
and cueing butter onto a deliberately overheated pan triggers the too-hot hold.**

| # | Step | Expected | ☐ |
|---|---|---|---|
| 19.1 | “Cook a food” → **▶ Smash Burgers ×4** | Recipe starts; preheat HOLD drives the target to 450 °F | ☐ |
| 19.2 | Preheat reaches band | Advances to “Add 2 patties + smash” (L2 cue card) | ☐ |
| 19.3 | Add patties | FOOD ADDED advances to the sear timer; “Flip + cheese” cue after | ☐ |
| 19.4 | Tap the cue bar at each step | Cue acknowledged; sequence advances | ☐ |
| 19.5 | 4 batches via the loop | Runs 4 batches then finishes (serial `[recipe] finished`) | ☐ |
| 19.6 | (Prep step) butter on a hot pan | “too hot for butter” hold until the pan cools to the add window | ☐ |

_Note: recipes are authored on-device from the built-in program now; the web
Recipe Creator (M20) adds custom JSON programs + prep-fat advice._

## M20 — Recipe Creator (web)

Advance env. **Acceptance (roadmap M20): the wizard generates a smash-burger
program from a food; the validator rejects a 700 °F hold, a poultry program with
the safety note stripped, and a butter-then-500 °F-sear; the page loads offline.**

| # | Step | Expected | ☐ |
|---|---|---|---|
| 20.1 | Browse **/creator** (Wi-Fi isolated from internet) | Page loads (no CDN); food list populates | ☐ |
| 20.2 | “New from food” → a food | Steps auto-generate (preheat → add → sides → remove) | ☐ |
| 20.3 | Validate a good program | “Valid ✓” | ☐ |
| 20.4 | Set a HOLD to 700 | “Invalid: hold above the 650F ceiling” | ☐ |
| 20.5 | PREP butter then HOLD 500 | Rejected (smoke point) | ☐ |
| 20.6 | Save a valid program | “Saved ✓”; persists in `/programs/*.json` (LittleFS) | ☐ |

---
# TRACK A — Closed-loop control (SSR box)

> ⚠️ **Mains + heat.** The SSR box switches line voltage into a resistive
> griddle. Build it per `hardware/panpilot_ssr_box.yaml` and §3.1.1 exactly:
> name-brand 40 A zero-cross SSR, mandatory heatsink + thermal compound, ~110 °C
> thermal fuse on the heatsink, 15 A line fuse, no user-accessible live parts,
> and set the griddle's own thermostat to MAX as an independent backstop.
> **Interlock truth-table tests (test_interlocks) must be green before any
> control runs on hardware** — they are.

## M14 — SSR box + actuator HAL + watchdog handshake

Build + flash the box (`hardware/panpilot_ssr_box.yaml`).

| # | Step | Expected | ☐ |
|---|---|---|---|
| 14.1 | Load test at full griddle draw for 1 h | Heatsink ≤ 70 °C; no discoloration | ☐ |
| 14.2 | Duty-cycle at the 3 s window | Griddle regulates smoothly; status LED blinks with duty | ☐ |
| 14.3 | **Kill Wi-Fi** | SSR forced OFF within 15 s (hardware watchdog), no PanPilot command | ☐ |
| 14.4 | **Kill the broker** | Firmware side: S7 trips (box liveness lost; S8 backs it up) → duty 0 + alert; box side: watchdog self-off | ☐ |
| 14.5 | Watchdog handshake | PanPilot refuses to arm until the box's retained `panpilot/ssr/status=online` is seen (enforced in `on_assist`; ceremony shows "no actuator" until then). Bench-verify the retained birth/LWT actually flips it | ☐ |
| 14.6 | **Unplug the MLX90640 while armed** | Duty forced to 0 within ~3 s (firmware frame-loss failsafe) + box watchdog backstop | ☐ |

## M14-D — DIRECT SSR (env:crowpanel35_advance_ssr, IO17 on UART1-OUT)

> **SUPERVISED BENCH ONLY.** This path has no independent hardware watchdog
> (unlike the SSR box). Fail-safe layers: interlocks -> frame-loss failsafe ->
> deadman timer -> crash-reboot leaves the pin Hi-Z. That last layer only
> works with the **required external 10 kΩ pulldown** across the SSR input.

| # | Step | Expected | ☐ |
|---|---|---|---|
| D-S.1 | Wire SSR+: IO17 (UART1-OUT "TX"), SSR−: GND, **10k pulldown across the SSR input**; power the board with the SSR load UNPLUGGED | Boot: `[ssr] direct GPIO actuator on IO17...`; pin stays LOW through boot (scope/LED) | ☐ |
| D-S.2 | Check the SSR's spec | DC input range includes 3.3 V (some Fotek-style want >3 V under load — verify it actually switches) | ☐ |
| D-S.3 | Arm via the ceremony (sensor connected) | Arming REFUSED without live frames; armed with them | ☐ |
| D-S.4 | Hold a duty, then **unplug the MLX** | Pin LOW within ~3 s (frame-loss failsafe + deadman) | ☐ |
| D-S.5 | Hold a duty, then hard-hang the firmware (flash a while(1) test build) | Pin LOW within SSR_DEADMAN_MS if RTOS lives, else on reboot/reset via the pulldown | ☐ |
| D-S.6 | STOP bar + each §3.3 interlock | Pin LOW immediately, exactly as the M15 rows | ☐ |

## M15 — Interlocks

`test_interlocks` (S1–S11 truth table) is green in `pio test`. On hardware,
demonstrate each S1–S11 row physically trips duty→0 (see §3.3 checklist):
confidence loss, pan removed, obstruction, runaway (boil-dry), max temp, sensor
fault, actuator heartbeat, comms loss, STOP bar, unattended 45 min, die > 85 °C.

## M16 — Closed-loop hold

| # | Step | Expected | ☐ |
|---|---|---|---|
| 16.1 | “Hold 350 °F”, walk away | Griddle held **350 ± 15 °F for 30 min** (time-proportional, pre-PID) | ☐ |

## M17 — PID + autotune wizard

| # | Step | Expected | ☐ |
|---|---|---|---|
| 17.1 | Enable PID | Hold tightens to **± 8 °F** | ☐ |
| 17.2 | Run “Tune this appliance” | Autotune completes on a griddle **and** a hot plate | ☐ |

## M18 — Controlled batch cooking

| # | Step | Expected | ☐ |
|---|---|---|---|
| 18.1 | 4 pancake batches under control | Recovery-to-ready **faster + more consistent** than the M6 advisory baseline (compare logs) | ☐ |

---
# BENCH ROUNDS — live-iteration features (2026-07)

## Map Burner wizard (per-pan knob calibration)

Needs the **real MLX + a real burner** (`crowpanel35_advance` build). On the
`_devsim` build the wizard runs against the scripted ramp for UI checks, but
**Save is blocked** (`bmapCanSave=false`, "SIMULATED data — cannot be saved"),
the same policy that blocks arming on dev builds.

| # | Step | Expected | ☐ |
|---|---|---|---|
| B.1 | My Pans → **Map burner** (needs ≥1 saved pan; button hidden when none) | Intro screen explains the ~5-min protocol; Start / Back | ☐ |
| B.2 | Follow the 5 prompts LOW→HIGH with the empty active pan | Each: "Set the burner knob to <NAME>, tap Ready" → 15 s settle + 30 s measure countdowns | ☐ |
| B.3 | "Turn the burner OFF, tap Ready" | 10 s settle + 30 s cool measure, then **Burner mapped!** with 5 predicted hold temps | ☐ |
| B.4 | Sanity-check the predictions | Monotonic LOW→HIGH; MED near where that burner actually holds a pan (compare a reference thermometer if handy) | ☐ |
| B.5 | **Save to pan**, then push past target until TURN DOWN NOW | Cue reads "aim knob at <NAME>" / "try <NAME>" using the **calibrated** suggestion, not the generic table; serial `[bmap] saved to pan '<name>'` | ☐ |
| B.6 | Reboot | Map persists (stored on the pan profile, NVS `profs3`); calibrated hints still used | ☐ |
| B.7 | Devsim build: run the wizard end-to-end | Completes, but Save disabled + "SIMULATED data" note; serial `[bmap] save REFUSED` if forced | ☐ |

## Settings Wi-Fi row + provisioning

| # | Step | Expected | ☐ |
|---|---|---|---|
| D.1 | Settings → **Wi-Fi** row, unprovisioned | Shows "tap to set up" | ☐ |
| D.2 | Tap it; join `PanPilot-XXXX` from a phone | Row flips to "join AP PanPilot-XXXX"; captive portal opens (SSID + password + optional MQTT broker / Web PIN) | ☐ |
| D.3 | Provision to your network | Row shows "<SSID> - panpilot.local"; `http://panpilot.local/` serves the dashboard; `/settings`, `/creator`, `/update` reachable | ☐ |
| D.4 | Reboot | Reconnects on its own; row shows the address without any portal | ☐ |
| D.5 | Let the first-boot portal time out (3 min), then Settings → Wi-Fi → tap | Portal reopens (serial: `[net] config portal reopened`) — no reboot needed | ☐ |

## Recipe program run (Smash Burgers x4)

| # | Step | Expected | ☐ |
|---|---|---|---|
| E.1 | Start the program (blue row in the food picker) with a food already selected | Any previously selected food is CLEARED; top-left shows "Smash Burgers x4" (not the old food/preset) | ☐ |
| E.2 | After preheat advances to "Add 2 patties + smash" | **No TOO HOT alarm** at the 450 °F hold (the program's setpoint carries between steps); bar shows "- tap when done ✓" | ☐ |
| E.3 | Tap the cue bar on an action step | Step advances (tap = "I did it"); food-drop auto-advance still works too | ☐ |
| E.4 | "Searing side 1" | Bar shows a live countdown (1:30 → 0:00); ONE chirp on entry, **no repeated beeping/strobe** during the timer | ☐ |
| E.5 | Action steps ("Flip + cheese") left unattended | These DO re-beep at L2 every 5 s until acted on (that's the point) | ☐ |

## Create-your-own: preset / food / program (bench 2026-07-12)

| # | Step | Expected | ☐ |
|---|---|---|---|
| G.1 | Preset picker → "+ New" card | Editor opens (name keyboard, lo/hi steppers, stainless); saved preset appears as a grey card | ☐ |
| G.2 | /creator → New food form → Save | Food appears in the device picker WITHOUT a reboot; starring it puts it on the top-level grid; a name+variant matching a built-in overrides it but can't lower safeInternalF | ☐ |
| G.3 | /creator → build + validate + save a program | It appears under the blue "Recipe programs" card (and the foods-list blue row) as "saved" | ☐ |
| G.4 | Run a saved program | Sequencer runs it (name top-left, cue bar steps); a hand-broken /programs/*.json is REFUSED with a serial/[log] reason (re-validated on load) | ☐ |
| G.5 | Pork-chop favorite card | Name/variant/temps each on their own line, nothing overflowing the card | ☐ |

## Alert-overlay context lines + dismissal

| # | Step | Expected | ☐ |
|---|---|---|---|
| C.1 | Pick a food (e.g. Eggs), heat to READY | Overlay shows **food name** above "READY" and "<temp> — add food, timer starts" below | ☐ |
| C.2 | Overshoot → TURN DOWN NOW / TOO HOT | Context line + action sub-line ("peak ~x — try <knob>", "<temp> — turn burner to LOW") | ☐ |
| C.3 | Let READY sit untouched | Overlay auto-dismisses after ~6 s back to the home screen (green READY action bar remains); home controls usable while the pan holds at temp | ☐ |
| C.4 | Tap a READY / TURN DOWN / ADD BATCH overlay | Dismisses immediately; re-appears only when the state changes and comes back (e.g. food-added dip → recovery → READY again) | ☐ |
| C.5 | TOO HOT (or battery-critical) overlay | **Not** dismissible by tap and never auto-hides — stays until the condition clears (safety) | ☐ |

---
# ENCLOSURE — v1 test print (hardware/enclosure/panpilot_enclosure.scad)

Print `part="shell"`, `"lid"`, `"base"` in PETG/ASA (not PLA — it lives over a
cooktop). Dimensions came from the factory STEP + the Waveshare drawing; these
are the assumptions a print has to confirm:

| # | Step | Expected | ☐ |
|---|---|---|---|
| F.1 | Drop the board into the shell | Envelope fits with the 0.6 mm clearance; glass window centered on the display | ☐ |
| F.2 | Lid + 4 longer **M2** screws into the corner standoffs | Threads match (factory ZM_M2 screws imply M2 — verify!); lid registration lip seats | ☐ |
| F.3 | USB-C cutout (right wall) | Both ports reachable with a cable | ☐ |
| F.4 | D55 in the sensor pod | PCB pocket fits; **hole dia assumed 2.5** for self-tap M2.5; lens centered in the aperture; PH2.0 tail reaches the cavity through the neck slots | ☐ |
| F.5 | Shell on the shelf base | Cradle rails capture it; screen ≈30° from vertical; stable when tapped | ☐ |
| F.6 | Aim check at mount height | Sensor sees the burner(s) (~42 × 25 cm footprint at 40 cm); thermal view confirms nothing clips the FOV cone | ☐ |

---
# SECONDARY BOARDS — CrowPanel Advance 5" (800×480 RGB)

> The 5" targets (`crowpanel_adv_5_v11`, `crowpanel_adv_5_v12`) are
> **compile-verified only**. The RGB display path uses LovyanGFX `Bus_RGB`/
> `Panel_RGB` (`src/hal/lgfx_panpilot_rgb.h`); the 480×320 UI is centered on the
> 800×480 panel (touch de-offset in `display.cpp`). The items below are **bench-
> gated** — none has been observed on glass. Pins/timings are captured from
> `../cyd-radio` (v1.1) and `../BladeKey-Overhead` (v1.2); flip the flagged
> values there if the panel misbehaves.

| # | Step | Expected | ☐ |
|---|---|---|---|
| 5.1 | Flash `crowpanel_adv_5_v11` (or `_v12`), power on | Boot banner names the 5" board; no crash | ☐ |
| 5.2 | **Backlight / reset** — the I/O-expander bring-up (`panel_power_on`) | Panel lit. If dark: the expander may be an **STC8 µC @ 0x30** (single-byte cmds), not the assumed TCA9534 @ 0x20/0x18 — see `board_pins.h` / BladeKey `Board.h` | ☐ |
| 5.3 | **Sync polarity / pclk** | Stable image. If black/rolling: flip `pclk_active_neg`, `hsync/vsync_polarity`, or `RGB_PCLK_HZ` in `lgfx_panpilot_rgb.h` / `board_pins.h` (v1.2 wants 21 MHz + `pclk_idle_high=1`) | ☐ |
| 5.4 | **Colours** | Reds/oranges correct (not cyan/blue). If swapped: the RGB byte/bit order needs adjusting for this panel | ☐ |
| 5.5 | UI centered on the 800×480 panel | 480×320 UI centered, black margins; **tap a target** → hits the right control (touch de-offset correct) | ☐ |
| 5.6 | MLX90640 on I²C 15/16 | Thermal view renders — sensor shares the touch bus (mutex), same as the 3.5" | ☐ |
