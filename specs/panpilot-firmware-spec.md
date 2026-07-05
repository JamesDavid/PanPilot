# PanPilot Firmware Specification — CrowPanel 3.5" + IR Thermal Array

**Target implementer:** Claude Code
**Build system:** PlatformIO (Arduino framework)
**Version:** 1.0 — derived from PanPilot MVP Spec, adapted for thermal camera matrix, laser module removed

---

## 1. Scope and Key Deltas from Parent Spec

This spec covers firmware only, for the Phase 0 → Phase 1.5 feature set of the PanPilot MVP, on exactly two pieces of hardware:

1. **Elecrow CrowPanel ESP32 3.5-inch HMI touchscreen panel** (display, touch, buzzer if onboard, USB-C power)
2. **MLX90640 IR thermal array camera** (32×24 pixels, I²C), replacing the MLX90614 single-pixel sensor

Deltas from the parent product spec:

- **No laser module.** All laser sections (parent spec §3.3, §10) are void. Aiming is done via a **live thermal image view** rendered on the display. The user physically adjusts the sensor head until the pan blob is centered in the thermal view. This is strictly better than a laser dot: the user sees exactly what the sensor sees.
- **No single-pixel spot temperature.** Pan temperature is derived from a **region of interest (ROI)** within the 32×24 frame (see §6). This enables real pan detection, pan-moved detection, food-added detection, and obstruction detection from image data rather than heuristics.
- **No VL53L1X distance sensor.** The thermal matrix subsumes its purpose (pan presence, obstruction, bump detection). Do not implement any ToF code.
- No Wi-Fi, no BLE, no cloud. Fully offline device.
- Battery support is out of scope for this firmware version; USB-C powered. Implement the display-dim/idle state machine anyway (§8) since it doubles as auto-wake UX.

Everything else in the parent spec (modes, presets, guidance language, thresholds) carries over and is restated here in implementable form.

---

## 2. Hardware Definition

### 2.1 Board — verify first, do not guess

Elecrow sells two distinct "CrowPanel 3.5" products with different MCUs and pinouts:

| Variant | MCU | Display | Touch |
|---|---|---|---|
| CrowPanel 3.5" (basic, DIS05035H) | ESP32-WROVER-B | ILI9488, 480×320, SPI | XPT2046 resistive, SPI |
| CrowPanel Advance 3.5" | ESP32-S3 (PSRAM) | ILI9488-class, 480×320 | Varies (may be capacitive) |

**Task zero for the implementer:** confirm which variant is on the bench, then pull the authoritative pin map from the Elecrow wiki page for that exact product/version and record it in `include/board_pins.h`. Do not trust pin numbers found in random GitHub repos or in this document. The values below are *placeholders known to match common Elecrow 3.5" basic demos* and must be verified:

```c
// include/board_pins.h  — VERIFY AGAINST ELECROW WIKI BEFORE FLASHING
#define TFT_MISO   12
#define TFT_MOSI   13
#define TFT_SCLK   14
#define TFT_CS     15
#define TFT_DC      2
#define TFT_RST    -1
#define TFT_BL     27   // backlight, PWM-capable
#define TOUCH_CS   33
#define TOUCH_IRQ  36
#define I2C_SDA    21   // exposed I2C header — verify; on Advance boards this differs
#define I2C_SCL    22
#define BUZZER_PIN -1   // set if external piezo added; some CrowPanels have none onboard
```

All hardware pins live in this one header. Nothing else in the codebase hardcodes a GPIO.

Provide **two PlatformIO environments** so either board builds from the same source:

```ini
; platformio.ini
[platformio]
default_envs = crowpanel35_basic

[env]
framework = arduino
monitor_speed = 115200
build_flags =
  -D LV_CONF_INCLUDE_SIMPLE
  -I include
lib_deps =
  bodmer/TFT_eSPI@^2.5
  lvgl/lvgl@^8.3
  adafruit/Adafruit MLX90640@^1.1
  adafruit/Adafruit BusIO

[env:crowpanel35_basic]
board = esp32dev            ; ESP32-WROVER-B, 8MB PSRAM variant if applicable
platform = espressif32
build_flags = ${env.build_flags}
  -D BOARD_CROWPANEL35_BASIC
  -D BOARD_HAS_PSRAM
  -mfix-esp32-psram-cache-issue

[env:crowpanel35_advance]
board = esp32-s3-devkitc-1  ; adjust to actual S3 module (flash/PSRAM size)
platform = espressif32
build_flags = ${env.build_flags}
  -D BOARD_CROWPANEL35_ADVANCE
  -D BOARD_HAS_PSRAM
```

TFT_eSPI configuration goes in build flags or a `User_Setup` override committed to the repo — never edited inside `.pio/libdeps`.

### 2.2 Sensor — MLX90640

- Part: **MLX90640ESF-BAB** (55°×35° FOV) preferred for 24–36" mounting distance. The BAA (110°×75°) variant also works but wastes pixels on the surroundings; the code must not assume either — FOV is a config constant used only for the on-screen spot-size hint.
- Interface: I²C, address `0x33`, run the bus at **400 kHz minimum**, 1 MHz if stable on the wiring (short cable from base to sensor head). Frame reads are large (~1.7 KB); at 100 kHz they will starve the UI.
- Refresh rate: configure the sensor for **8 Hz**, chess-pattern mode. Each full thermal frame requires two subpage reads; effective full-frame rate is 4 Hz, which is plenty for pan thermodynamics.
- Emissivity: default **0.95** (seasoned cast iron, carbon steel, oiled stainless, nonstick are all ≈0.9–0.95). Expose per-profile emissivity override (0.10–1.00) in settings for the bare-stainless case, plus a per-profile flat temperature offset.
- Geometry at 55° FOV (for UI hints only): full frame width ≈ 25" at 24" distance, ≈ 31" at 30", ≈ 37" at 36". A 10" pan therefore spans roughly 10–13 pixels across — comfortably enough for blob detection and an interior ROI.

### 2.3 Buzzer

If the verified board has no onboard buzzer, drive an external passive piezo on `BUZZER_PIN` with LEDC PWM. All alert sounds route through one `Buzzer` module with a mute flag persisted to NVS. Distinct patterns (short chirp / double chirp / long / SOS-ish repeat) for: target reached, turn-down-now, overheat, pan lost, heating detected wake.

---

## 3. Project Structure

```
panpilot/
  platformio.ini
  include/
    board_pins.h
    app_config.h          # tunables: thresholds, rates, timeouts (all §6–§9 constants)
    lv_conf.h
  src/
    main.cpp              # setup, task spawn only
    sensor/
      mlx90640_source.cpp/.h   # frame acquisition task, produces ThermalFrame
      frame_analysis.cpp/.h    # blob/ROI extraction, PanReading production
    core/
      thermal_model.cpp/.h     # smoothing, rate, ETA, overshoot prediction
      guidance.cpp/.h          # state machine: guidance states + preset logic
      events.cpp/.h            # pan placed/removed/moved, food added, obstruction
      presets.cpp/.h           # built-in preset table + custom target
      profiles.cpp/.h          # Learn Pan Mode data model (Phase 1.5)
      session.cpp/.h           # session summary accumulation
    ui/
      ui_root.cpp/.h           # LVGL init, screen manager
      screen_home.cpp/.h
      screen_thermal.cpp/.h    # live thermal view (aiming + ROI)
      screen_presets.cpp/.h
      screen_settings.cpp/.h
      screen_ready.cpp/.h      # full-screen READY / TURN DOWN / TOO HOT states
      theme.cpp/.h
    hal/
      buzzer.cpp/.h
      backlight.cpp/.h
      storage.cpp/.h           # NVS wrapper (Preferences)
  test/
    test_thermal_model/        # native unit tests, see §11
    test_frame_analysis/
    test_guidance/
```

Rule: `core/` and `sensor/frame_analysis` contain **no Arduino or LVGL includes** — pure C++ operating on plain structs, unit-testable with `pio test -e native`. Add a `[env:native]` test environment.

---

## 4. Concurrency Model

FreeRTOS tasks (pinning shown for dual-core; on any variant keep sensor I/O off the LVGL core):

| Task | Core | Rate | Role |
|---|---|---|---|
| `SensorTask` | 0 | 8 Hz subpages | Read MLX90640 subpages, assemble full frames, run `frame_analysis`, publish `PanReading` to a queue |
| `LogicTask` | 0 | 4 Hz | Consume `PanReading`, update `thermal_model`, `events`, `guidance`, `session`; publish `AppState` snapshot |
| `UITask` | 1 | `lv_timer_handler` every 5 ms | LVGL render + touch; reads latest `AppState` snapshot (mutex or double-buffer, never blocks on sensor) |

Data flow is one-directional: SensorTask → LogicTask → UITask. UI sends user intents (preset selected, target changed, mute, mode change) back through a command queue to LogicTask. No shared mutable state outside these two channels.

Core structs:

```c
struct ThermalFrame {           // raw-ish sensor output
  float px[24][32];             // °C per pixel
  float ambientC;               // sensor die/ambient
  uint32_t t_ms;
  bool valid;
};

struct PanReading {             // output of frame_analysis
  float panTempC;               // robust ROI statistic (§6.2)
  float roiMinC, roiMaxC;
  float backgroundC;            // scene excluding ROI
  uint8_t roiPixelCount;
  float roiCx, roiCy;           // blob centroid in pixel coords
  uint8_t confidence;           // 0–100, §6.3
  PanPresence presence;         // ABSENT / PRESENT / OBSTRUCTED / UNCERTAIN
  uint32_t t_ms;
};

struct AppState {               // what the UI renders
  Mode mode; const Preset* preset;
  float displayTempF; float rateFPerMin;
  int etaSeconds;               // -1 = unknown/estimating
  float projectedPeakF;         // NAN if none
  GuidanceState guidance;       // §7.2 enum
  EventFlag lastEvent;
  PanReading lastReading;       // for thermal screen + confidence icon
  bool muted; ...
};
```

Units: all internal math in °C; conversion to °F only at the display layer. Default display unit °F, toggle in settings, persisted.

---

## 5. Sensor Acquisition (`sensor/mlx90640_source`)

- Init: probe 0x33, load EEPROM calibration via Melexis/Adafruit driver, set chess mode + 8 Hz. On init failure show a persistent "SENSOR NOT FOUND — check cable" screen and retry every 2 s; never boot into a fake reading.
- Per-frame: reject frames containing NaN/inf or any pixel outside −40…+600 °C; count consecutive bad frames, surface `SENSOR FAULT` after 10.
- Apply active emissivity setting at the driver level.
- Publish every assembled full frame (4 Hz) to `frame_analysis`; do not smooth here.
- Track sensor die temperature; if it exceeds 85 °C, raise a `DEVICE TOO HOT — move away from heat` warning (parent spec §14.4).

---

## 6. Frame Analysis — ROI, Pan Detection, Confidence

This module replaces the laser, the ToF sensor, and most of the parent spec's aim heuristics.

### 6.1 Pan blob detection

Per frame:

1. Compute scene background estimate = median of all 768 pixels.
2. Threshold: candidate pixels where `px > background + PAN_DELTA_C` (default 10 °C) **or**, during active cooking, `px > 60 °C` absolute.
3. 4-connected component labeling on the 32×24 grid (trivial at this size; simple two-pass or flood fill).
4. Pan blob = largest component with area ≥ `MIN_PAN_PIXELS` (default 6). If none and the pan was recently hot, fall back to tracking the previously locked ROI position (a cooling pan can drop below the delta threshold — use absolute `> ambient + 8 °C` inside the last-known ROI before declaring absence).

### 6.2 Pan temperature statistic

Never report the single max pixel (flame licks, hot spots) and never the blob mean (edge pixels are mixed pan/background). Use:

- **Erode the blob by one pixel** (drop any blob pixel with a non-blob 4-neighbor) to get the interior; if erosion empties the blob, use the full blob.
- `panTempC` = **75th percentile** of interior pixels. Also report min/max of interior for the UI hotspot indicator.

### 6.3 ROI lock and confidence

- **Auto mode (default):** ROI follows the detected blob each frame, with centroid low-pass filtering to avoid jitter.
- **Tap-to-lock:** on the thermal screen the user can tap the pan; lock a circular ROI (radius = fitted blob radius) at that spot. Locked ROI still tracks slow centroid drift (< 1 px/frame) but flags fast jumps as PAN MOVED.
- **Confidence (0–100)**, drives an icon on every screen and gates guidance strength:
  - blob area within expected range for locked profile: +
  - centroid stable over last 2 s: +
  - interior temperature spread (p90 − p10) small: +
  - frame-to-frame panTemp noise low: +
  - Reflective-stainless signature: high spatial variance + implausibly low reading vs. heating time → cap confidence at 50 and set the `STAINLESS?` hint flag (see §7.5).
- **Presence classification:**
  - `ABSENT`: no qualifying blob for > 3 s (subject to the cooling fallback above).
  - `OBSTRUCTED`: blob suddenly replaced by large near-body-temperature (25–40 °C) region overlapping ROI → hand/lid/spatula; suppress alerts, show "checking…" — do not fire PAN REMOVED for the first 5 s of obstruction.
  - `UNCERTAIN`: confidence < 30.

### 6.4 Events derived from frames (`core/events`)

| Event | Detection |
|---|---|
| PAN PLACED | ABSENT→PRESENT transition, blob persists 2 s |
| PAN REMOVED | PRESENT→ABSENT persists 3 s (hot blob vanishes, no obstruction signature) |
| PAN MOVED / CHECK AIM | centroid jump > 4 px between frames, or blob partially clipped at frame edge for > 2 s |
| FOOD ADDED | panTemp drops ≥ `FOOD_DROP_C` (default 15 °C) within ≤ 3 s while blob area stable/grows and burner presumably unchanged |
| OBSTRUCTION | §6.3 signature |

All thresholds live in `app_config.h`.

---

## 7. Thermal Model and Guidance (`core/`)

### 7.1 Smoothing, rate, ETA, overshoot

- **Display temperature:** exponential smoothing of `panTempC`, α such that time constant ≈ 2 s at 4 Hz. Keep the raw series (ring buffer, last 60 s at 4 Hz = 240 samples) for event detection and rate fitting.
- **Rate (°/min):** least-squares slope over the last 10 s of raw samples, recomputed each logic tick; report NAN until ≥ 5 s of PRESENT data. Classify per parent spec §6.2: stable < ±5 °F/min; slow 5–20; medium 20–60; fast > 60 (configurable).
- **ETA:** `eta = (target − current) / rate` when rate > 5 °F/min toward target and confidence ≥ 50; else show "estimating…". Clamp displayed ETA to 20 min.
- **Overshoot prediction:** projected peak = `current + rate × LAG_MINUTES`, where `LAG_MINUTES` defaults to 0.5 (30 s of thermal lag) and is replaced by the learned lag when a pan profile is active. If projected peak > target-range top while still below target → guidance = TURN DOWN NOW with `Projected peak: XXX°F`. This is the flagship feature; unit-test it thoroughly (§11).

### 7.2 Guidance state machine

States (superset of parent spec §4.1B): `IDLE, NO_PAN, HEAT_MORE, HOLD, TURN_DOWN_SOON, TURN_DOWN_NOW, READY, TOO_HOT, COOLING, RECOVERING, CHECK_AIM, LOW_CONFIDENCE`.

Transition rules (target range [lo, hi], warn = overheat threshold):

- below lo, rate ≤ slow → HEAT_MORE
- below lo, rising, projected peak ≤ hi → HOLD (with ETA)
- below lo, projected peak > hi → TURN_DOWN_NOW
- within 15 °F below lo, rising medium/fast → TURN_DOWN_SOON
- in [lo, hi] → READY (buzzer once on entry; re-arm only after leaving the range by > 10 °F)
- hi < temp < warn → TURN_DOWN_NOW
- ≥ warn → TOO_HOT (buzzer pattern repeats every 10 s until below hi or muted)
- falling below lo after having been READY → COOLING, or RECOVERING if FOOD ADDED was detected (Recovery Monitor, §7.4)
- presence/confidence problems override: ABSENT → NO_PAN, MOVED → CHECK_AIM, UNCERTAIN → LOW_CONFIDENCE (guidance text softens: "trend: rising" instead of absolute instructions)

Hysteresis: every threshold crossing needs 2 consecutive logic ticks to fire; alerts have per-state re-arm rules to prevent buzzer spam.

### 7.3 Presets

Built-in table exactly per parent spec §19 (Eggs, Pancakes, Stainless Preheat, Sear, Tortillas/Quesadillas, Generic) with target range, warn threshold, recovery-monitor flag, and a `stainlessHints` flag on Stainless Preheat. Generic target adjustable in 5 °F steps, ready window ±10 °F default. Custom target and last preset persist to NVS.

### 7.4 Recovery Monitor (Phase 1.5)

Active when preset has recovery enabled and FOOD ADDED fires: record pre-drop temp, watch climb back into target range, then buzz + `Pan recovered — add next batch`. Warn `Recovery slow — raise heat?` if projected recovery > 3 min; `Recovering fast — watch heat` if rate > fast threshold.

### 7.5 Stainless handling

When confidence is capped with the `STAINLESS?` flag or the Stainless Preheat preset is active: banner `Bare stainless reads low — trend is reliable, add oil/water for true temp`, emphasize the rate/trend line visually over the absolute number, and never fire TOO_HOT on absolute value alone below 300 °F indicated (avoid false alarms from reflections).

---

## 8. Power / Idle State Machine

USB-powered, but implement:

- **ACTIVE:** backlight full (PWM via `TFT_BL`), 8 Hz sensor, UI live.
- **IDLE:** entered after 10 min with no touch and scene ≈ ambient; backlight to 10 %, sensor drops to one frame per 10 s, LVGL paused except a clockless "PanPilot — monitoring" screen.
- **Wake:** touch, or heat event — `panZone > ambient + 11 °C (≈20 °F)` with positive slope across 3 consecutive samples → ACTIVE, single chirp, `Heating detected — select target or preset` prompt (parent spec §7.2).
- **Session end:** temp within 8 °C of ambient and stable 10 min → store session summary (max temp, time-in-range, overheat seconds, profile used) to a small NVS ring log (last 10 sessions), return to IDLE.

---

## 9. UI Specification (LVGL 8.3, 480×320 landscape)

General: dark theme, huge numerals, kitchen-distance readability. Minimum touch target 60×60 px. Top status bar on every screen: mode name, confidence icon, mute icon.

### 9.1 Home / cooking screen
Layout per parent spec §5.1: current temp dominates (≈ 120 pt), target below it, rate + trend arrow, ETA line, and a full-width color-coded action bar at the bottom (state → text/color: HOLD = blue, TURN DOWN NOW = orange, READY = green, TOO HOT = red flashing). Tapping the temp opens the thermal view; tapping the target opens the target adjuster (+/− 5 °F buttons and slider).

### 9.2 Thermal view (replaces all laser/aim screens)
- 32×24 frame rendered as a color-mapped heat image scaled to ~384×288 (12× nearest-neighbor is fine; optional 2× bilinear if cheap), ironbow or similar palette, auto-scaled to scene min/max with the current range printed.
- Overlays: blob outline, ROI circle, crosshair at frame center, `panTemp` label pinned to the blob.
- Tap = lock ROI at tap point; "Auto" button clears the lock. "Done" returns home.
- This screen doubles as the aiming workflow: user physically points the head until the pan blob sits under the crosshair. Show live hint text: `Center the pan in view` → `Good aim ✓`.

### 9.3 Other screens
- **Preset picker:** grid of large cards (6 built-ins + Custom).
- **Full-screen alert states:** READY / TURN DOWN NOW / TOO HOT / PAN MOVED, matching parent spec §5.2–5.5 copy, auto-dismiss back to home when the state clears.
- **Settings:** units, brightness, mute, emissivity, stainless mode, thresholds reset, about/version.
- Learn Pan Mode wizard (Phase 1.5): step-driven flow per parent spec §8.2, recording rate per burner setting into a `PanProfile` stored in NVS; profile guidance then feeds `LAG_MINUTES` and burner-setting suggestions (`Set burner to MEDIUM`, `Switch to LOW in 20 sec`).

---

## 10. Persistence (`hal/storage`, ESP32 Preferences/NVS)

Namespaces: `cfg` (unit, brightness, mute, emissivity, custom target, last preset, thresholds if user-modified), `profiles` (up to 8 `PanProfile` blobs, versioned struct with magic + version byte), `sessions` (ring of 10 summaries). All structs serialized with explicit version tags; on version mismatch, discard and use defaults — never crash on stale NVS.

---

## 11. Testing

- `pio test -e native` must pass with no hardware.
- `test_thermal_model`: smoothing convergence; slope fitting against synthetic ramps with noise; ETA math; **overshoot prediction table tests** (given ramp + lag → fires TURN_DOWN_NOW at the correct temperature, does not fire on slow ramps).
- `test_frame_analysis`: synthetic 32×24 frames — centered disc, off-center disc, edge-clipped disc, two blobs, hand-over-pan obstruction, no pan, stainless-like noisy blob. Assert blob selection, interior percentile value, presence classification, MOVED/FOOD ADDED event firing.
- `test_guidance`: scripted `PanReading` sequences through full cook scenarios (preheat-overshoot, eggs happy path, sear + batch recovery, pan removed mid-cook) asserting the guidance state trace.
- On-target smoke test doc lives in the repo as `HARDWARE_TEST.md` checklist (boot, sensor found, thermal view live, touch, buzzer, NVS roundtrip).

---

## 12. Milestones (implement strictly in order)

**M0 — Board bring-up.** Both PIO envs compile; verified `board_pins.h`; LVGL "hello" with touch; backlight PWM; buzzer beep. *Accept:* touch a button, hear a beep.

**M1 — Thermal pipeline.** MLX90640 at 8 Hz chess; thermal view screen with palette + tap-to-lock ROI; serial dump of `PanReading`. *Accept:* hot mug visible and tracked on screen; panTemp within ±5 °C of a reference thermometer on cast iron.

**M2 — Thermometer Mode.** Home screen: smoothed temp, trend arrow, rate; NO_PAN / CHECK_AIM states; unit toggle + NVS. *Accept:* stable non-jumpy display while heating a pan.

**M3 — Target Assist.** Target adjuster, ETA, guidance state machine, READY/TOO_HOT alerts with buzzer + hysteresis, full-screen alert states. *Accept:* end-to-end preheat of cast iron to 350 °F with correct HOLD → TURN_DOWN_SOON → READY sequence.

**M4 — Overshoot prediction + presets.** Projected-peak logic, TURN DOWN NOW with predicted peak, preset picker with §7.3 table, stainless hints. *Accept:* on a fast HIGH-burner ramp, TURN DOWN NOW fires before target and predicted peak is within ±25 °F of the actual peak.

**M5 — Auto-wake + sessions.** Idle/wake state machine, heating-detected chirp + prompt, session summaries. *Accept:* device dims after cook, wakes by itself when a burner is lit.

**M6 (Phase 1.5) — Learn Pan Mode + Recovery Monitor.** Wizard, profiles in NVS, profile-driven lag + burner suggestions, FOOD ADDED → recovery flow. *Accept:* pancake batch test — device announces `Add next batch` correctly for 3 consecutive batches.

---

## 13. Non-negotiables / style

- No blocking delays in tasks; no work in ISRs beyond flag-setting.
- No dynamic allocation in the per-frame path after init.
- Every tunable constant in `app_config.h` with a comment stating units and the parent-spec section it came from.
- Firmware version string (semver + git hash via PIO build script) shown on Settings/About and printed at boot.
- Serial log levels via `esp_log`; per-frame spam only at VERBOSE.
