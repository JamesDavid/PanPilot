# PanPilot Roadmap Specification — Phase 2 Through Ultimate Goal

**Extends:** `panpilot-firmware-spec.md` (M0–M6, Phases 0–1.5)
**Hardware baseline:** CrowPanel 3.5" + MLX90640 thermal array, no laser; Phase 3 adds the PanPilot SSR box
**Version:** 1.4 — Prep steps (butter/oil/liquid) with thermal melt detection, smoke-point tracking, and authoring-time fat advice; preplib added as Appendix A.1

---

## 0. Roadmap at a Glance

| Phase | Name | One-line promise | Milestones |
|---|---|---|---|
| 0–1 | Bench + Kitchen MVP | See pan temp, hands-free | M0–M3 |
| 1.5 | Smart Guidance | Better than any IR gun: predicts, learns, coaches | M4–M6 |
| **2** | **Product-Like Prototype** | **A finished-feeling device: battery, connected, logged, polished** | **M7–M12** |
| **3** | **Closed-Loop Control** | **Advisor on any stove; autopilot on the SSR-boxed griddle** | **M13–M18** |
| **4** | **Cooking Flight Director (ultimate goal)** | **PanPilot runs the cook; the human executes steps on cue** | **M19+ / research** |

The through-line: Phase 1.5 answers *"what should I do right now?"* Phase 3 removes the human from the temperature loop. Phase 4 removes the human from the *decision* loop — temperature, timing, and sequencing are all managed; the person just does the physical steps when told.

## 0.1 One Device, Two Configurations

From Phase 3 onward, the same firmware runs in two configurations, selected simply by whether a heat actuator is armed:

| | **Stovetop Advisor** | **Griddle Autopilot** |
|---|---|---|
| Cooktop | Any — gas, electric coil, induction, the user's existing range | Electric griddle (or hot plate) through the PanPilot SSR box |
| Who turns the heat | **The human**, on cue: "TURN TO MEDIUM," "TURN DOWN NOW" via screen flash + buzzer patterns (§3.5) | **PanPilot**, via SSR duty-cycling under interlocks (§3.3) |
| Temperature loop | Human-in-the-loop; device predicts, instructs, verifies compliance | Fully closed; ±8–15 °F holds, walk-away preheat |
| Recipe programs (Phase 4) | Work fully — programs cue knob changes instead of setting duty | Work fully — programs set setpoints directly |
| Mode name in firmware | `ADVISORY` | `ASSIST` |

This duality is the product story: mounted over the family range it's a coach that beeps and flashes you through pancakes, omelettes, and steak; pointed at a $40 electric griddle with the SSR box inline, the same device just does it. Nothing about the perception, guidance, preset, or recipe layers differs between the two — the actuator is the only variable.

**Autopilot gating rule (explicit):** `ASSIST` is available **only** when a `HeatActuator` is discovered, watchdog-handshaken, and armed through the §3.3 ceremony — in practice, the SSR box (§3.1.1) or a watchdog-capable smart plug. With no actuator present, every control-related UI element is hidden entirely (not grayed out); the device is purely `ADVISORY` and there is no code path to energize anything. Firmware must treat "actuator present" as a runtime discovery, never a build flag — one binary serves both configurations.

## 0.2 Build Order & Dependencies

Phases are **capability tiers, not a forced build sequence.** The actual dependency graph:

```
M0–M6 (base spec) ──► Phase 2 core (M7–M12)
                            │
        ┌───────────────────┼──────────────────────┐
        ▼                   ▼                      ▼
  M13 Attention       M12.5 Food timer        Track A: Control
  (no Phase-3 deps;   (requires M13!)         M14 SSR box → M15 interlocks
   build during                │               → M16 hold → M17 PID → M18
   Phase 2)                    ▼
                        Track B: Recipe sequencer (§4.1)
                        (requires M13 + M12.5 + guidance —
                         does NOT require Track A)
                                │
        Track A + Track B ──────┴──► Phase 4 autopilot recipes,
                                     then 4.2 ML research
```

Answers to the obvious sequencing questions:

- **Does Phase 4 need to come before Phase 3? No — and neither strictly precedes the other.** The recipe sequencer (§4.1) is *Phase 3-independent*: in Advisor configuration it cues knob turns and runs food timers with zero control hardware. It's legitimate — arguably smart — to build Track B before or in parallel with Track A, since Track B needs no new hardware and delivers the "flight director" feel on the existing stove immediately. Only §4.2 (ML doneness research) genuinely benefits from waiting: Phase 3's controlled holds produce cleaner training traces.
- **M13 (attention escalation) is listed under Phase 3 for narrative reasons but has no Phase 3 dependencies** and is a hard prerequisite of M12.5 (the food timer's FLIP cues ride on it). Build it during Phase 2, immediately after M11.
- **Recommended order for this project:** M7–M11 → M13 → M12.5 → M12 → then fork: Track B (recipe sequencer on the stovetop, no new hardware) while the SSR box is being physically built, then Track A milestones as the box comes online.

---

# PHASE 2 — Product-Like Prototype

Goal: a unit you could hand to someone else's household for a week without apologizing. Everything in this phase is implementable now on the existing hardware plus small additions.

## 2.1 Power & Battery Subsystem

The CrowPanel 3.5" basic has no LiPo charge circuit; the Advance variant may. Spec both paths behind one HAL:

**Hardware additions (if board lacks native support):**
- 1S LiPo 2000–3000 mAh flat pack
- IP5306 or TP4056+DW01 charge/protect board fed from a second USB-C, or inline with panel 5 V — implementer picks based on measured panel current
- MAX17048 fuel gauge on I²C (preferred over voltage-divider guessing; shares the sensor bus)
- P-MOSFET high-side switch on `SENSOR_PWR_EN` GPIO to power-gate the MLX90640 in deep idle (it draws ~20 mA continuously; gating it is the single biggest idle win)

**Firmware (`hal/power`):**
- Power source detection (USB present vs battery), charging state, SoC % from fuel gauge, low-batt (15 %) and critical (5 %) events
- Extend §8 state machine of the base spec:
  - **ACTIVE** — as before; full brightness on USB, 70 % on battery
  - **IDLE** — backlight 10 %; on battery, MLX drops to one power-gated wake-read every 20 s (re-init after power-up takes ~80 ms + first frame; measure and tune)
  - **DEEP IDLE (battery only)** — display off, ESP32 light sleep, timer wake every 60 s → power sensor → single frame → heat check → back to sleep. Touch wake via `TOUCH_IRQ` as EXT wake source
  - **SHIP MODE** — long-press power-off from settings; only USB insertion or reset wakes
- Budget target: ≥ 7 days standby on 2500 mAh, ≥ 6 h continuous active cooking. Add a `power_audit` debug screen showing measured rail currents/estimates.
- Critical battery during an **active cook with alerts pending** must not silently die: fire a distinct triple-buzz + full-screen "PLUG ME IN" before shutdown.

## 2.2 Connectivity (local-first, cloud-never)

Wi-Fi joins the product in Phase 2, but as a convenience layer — every cooking feature must keep working with Wi-Fi off. No accounts, no cloud dependency.

**Stack:**
- Provisioning: on-device settings screen with SSID scan + touch keyboard; fallback SoftAP `PanPilot-XXXX` captive portal for first setup
- mDNS: `panpilot.local`
- **Embedded web UI** (ESPAsyncWebServer, static assets gzipped in LittleFS):
  - Live dashboard: current temp, guidance state, ETA — WebSocket push at 2 Hz
  - **Live thermal view in the browser**: 32×24 frame as JSON over WebSocket at 2 Hz, rendered client-side to a canvas with the same palette as the device. This makes remote aiming/verification possible and is a killer demo
  - Settings mirror (units, presets, thresholds, emissivity, profiles CRUD)
  - Session log browser + CSV download
  - **Recipe Creator** (§4.1.1) — mounts here once Track B lands; the web server, REST scaffolding, and LittleFS asset pipeline built in this phase are its foundation
- **MQTT + Home Assistant auto-discovery** (`core/ha_bridge`):
  - Entities: pan temperature (sensor), rate (sensor), guidance state (sensor), presence (binary_sensor), events (device triggers: ready, too_hot, food_added, pan_removed), mute (switch), target temp (number), active preset (select)
  - Discovery topics per HA MQTT conventions, retained; availability topic with LWT
  - This is the bridge that Phase 3 rides on (smart-plug control loops via the same broker), so build it clean
- **OTA:** `esp_https_ota`-based pull from a URL configured in the web UI, plus browser-upload fallback. Dual-app partition table with rollback on boot-loop (3 failed boots → revert). Version + channel shown in About.
- Time: SNTP when connected; RTC-less timestamping degrades gracefully to relative session time offline.

**Security floor (even for a home prototype):** web UI auth optional but supported (single PIN), OTA URL HTTPS-only, no telnet/raw sockets, Wi-Fi creds in NVS encrypted-at-rest if flash encryption is enabled later. Document the threat model as "hostile housemate," not nation-state.

## 2.3 Data, Logging, Sessions

- Move session storage from NVS ring (base spec §10) to **LittleFS**: one compact binary record per session + optional 1 Hz temperature trace (~4 KB per 30-min cook). Cap at 2 MB with oldest-first eviction.
- Session record: start/end time, preset/profile, max temp, time-to-target, time-in-range, overheat seconds, food-added count, recovery times, min confidence, battery delta.
- On-device "Last Cook" screen: sparkline of the trace (LVGL chart), key stats. Web UI shows full history.
- Export: CSV per session over HTTP.

## 2.4 Multi-Pan / Multi-Zone (matrix dividend)

The 32×24 array can see two adjacent burners at 30–36" with the 55° part. Phase 2 adds:

- Blob tracker generalized to **top-2 blobs** with persistent IDs (nearest-centroid association frame-to-frame)
- UI: primary pan full-size, secondary as a corner tile with temp + trend; tap to swap primary
- Independent target per zone; guidance engine instantiated per tracked pan
- Thermal view labels blobs A/B; tap-to-lock assigns which blob is which zone
- Acceptance: two pans on front burners, simultaneous eggs (300 °F) + sear preheat (500 °F), correct independent READY alerts

## 2.5 UX Polish

- First-boot onboarding wizard: mount → aim via thermal view → pick unit → done (≤ 90 s)
- Preset editor on device and web (clone built-in → adjust ranges)
- Alert sound designer: 3 volume levels + per-event enable, quiet hours
- Brightness auto-dim on ambient (reuse scene background temp as a crude proxy is not acceptable — add nothing; manual + schedule only, keep scope tight)
- Localization scaffolding (string table), °C-first defaults when unit = °C
- Accessibility pass: max-contrast theme, all alerts have distinct sound patterns (device usable without reading the screen)

## 2.6 Enclosure v2 (mechanical requirements the firmware must respect)

Parametric OpenSCAD, printed PETG/ASA:

- Two-piece: display base (panel, battery, buzzer, magnets + silicone pad) and sensor head (MLX90640 recessed ≥ 6 mm behind a replaceable grease shield aperture — **no window over the sensor**; any film/PE window attenuates LWIR unpredictably. Shield = sacrificial snap-in aperture ring you wash)
- Ball joint with friction lock between head and base arm; head cable = 4-wire (3V3 gated, GND, SDA, SCL) with strain relief both ends, ≤ 30 cm for 1 MHz I²C margin
- Firmware hook: `internal die temp > 85 °C` warning already exists; add enclosure NTC pad provision on a spare ADC pin for battery-compartment temperature (charge inhibit > 45 °C)
- Wipeable front, no exposed pins, magnet pull force spec ≥ 4× device weight

## 2.7 Integrated Food Timer & Cook-Time Database

Presets say *how hot*; the food database says *how long*. Combined with FOOD ADDED perception, PanPilot runs per-side cook timers that start themselves, adjust to actual pan temperature, and cue flips through the attention system. This is the bridge between presets (Phase 1.5) and full recipe programs (Phase 4).

**Data model (`core/foodlib`):**

```c
struct FoodEntry {
  const char* name;             // "Pancakes", "Smash Burger", "Ribeye"
  const char* variant;          // "4-inch", "1in thick — medium rare", ""
  uint16_t panTargetF_lo, hi;   // links to or overrides preset target
  uint16_t sideSec[4];          // per-side seconds at reference pan temp
  uint8_t  sides;               // 1 = no flip (sunny eggs), 2 = flip once, up to 4
  uint16_t refTempF;            // pan temp the times were authored at
  int8_t   tempCompPctPer25F;   // % time change per 25 °F deviation (default -12)
  uint16_t restSec;             // post-remove rest cue (steak), 0 = none
  uint16_t safeInternalF;       // 0 = n/a; 165 poultry, 160 ground meat, 145 whole cuts/fish
  const char* flipHint;         // "flip when edges set and bubbles pop"
};
```

**Seed database (~25 entries, compiled in):** pancakes, French toast, eggs (sunny / over-easy / scrambled / omelette), smash burger, ⅓-lb burger, steak (rare / medium-rare / medium × 1" and 1.5" thickness), bacon, sausage links, chicken breast (butterflied), chicken thigh, pork chop, salmon, white fish, shrimp, grilled cheese, quesadilla, tortilla, hash browns, tofu. Times authored at each entry's reference pan temperature from standard culinary sources; every entry human-reviewed, none invented by the implementer.

**Timer engine (`core/foodtimer`):**
- **Auto-start:** FOOD ADDED event while a food is selected starts the side-1 timer (manual tap-to-start fallback always available). PAN REMOVED pauses; OBSTRUCTED does not.
- **Cues via the attention system (§3.5):** `FLIP` at L2 with the entry's flipHint as the sub-line; `REMOVE` at L2; rest countdown completes at L1. Multi-item batches integrate with the recovery monitor and show a batch counter.
- **Temperature-compensated timing (the differentiator):** the countdown is a *doneness accumulator*, not wall-clock. Progress accrues as `Δt × k(panTemp)` where k = 1 at refTempF and scales by tempCompPctPer25F, clamped 0.7–1.3. A pan running 40 °F cold visibly stretches the remaining time, with a banner explaining why. A phone timer can't do this; a device that watches the pan can.
- **Post-cook feedback loop:** after REMOVE, an optional one-tap "How was it? Under / Perfect / Over" nudges that food's stored time ±8 % persistently (per-user, per-food; plain arithmetic, no ML). Feeds the session record.
- Multi-zone (§2.4): one independent timer per tracked pan.

**Food-safety rule (non-negotiable):** entries with `safeInternalF` set (poultry, ground meat, pork, fish) display *"Surface timing only — verify XXX °F internal"* on the REMOVE cue. Thermal surface data can never claim internal doneness; Phase 4 BLE-probe fusion (§4.3) is the upgrade path that can.

**Storage & editing:** seed table in flash — **the complete authored seed database is Appendix A of this document** (`foodlib_seed.h`, 28 entries, drop-in for `core/foodlib/`); user overrides and custom foods as versioned JSON in LittleFS, editable in the Phase 2 web UI. Feedback nudges live with the overrides.

**UI:** food picker extends the preset picker (cards show temp + total time); the cooking screen gains a countdown arc around the temperature numeral, a side indicator (1/2), and batch count.

## 2.8 Phase 2 Milestones

- **M7 — Power HAL + battery states.** Accept: unplug mid-cook, device keeps guiding, correct SoC, deep-idle standby current measured ≤ 3 mA average.
- **M8 — Wi-Fi + web dashboard.** Accept: live temp + thermal view in a phone browser via `panpilot.local` during a real cook.
- **M9 — MQTT/HA bridge.** Accept: entities auto-appear in Home Assistant; an HA automation ("flash kitchen lights on TOO_HOT") works.
- **M10 — OTA + dual partition.** Accept: flash a deliberately boot-looping image; device self-reverts.
- **M11 — LittleFS sessions + web history.** Accept: 5 cooks logged, traces render, CSV opens in a spreadsheet.
- **M12 — Multi-pan.** Accept: §2.4 two-pan scenario passes 3 consecutive runs.
- **M12.5 — Food timer + cook-time database.** Accept: pancake cook auto-starts on pour (FOOD ADDED), FLIP cue fires at the compensated time, batch counter tracks 3 batches; chicken entry shows the internal-temp verification note on REMOVE; a deliberately cold pan (-40 °F from target) visibly extends the countdown with the explanatory banner.

---

# PHASE 3 — Closed-Loop Control

Goal: PanPilot stops saying "turn down now" and starts doing it. The human still starts the cook, loads the pan, and can always override.

## 3.1 Actuation Targets (strict order)

1. **PanPilot SSR box + electric griddle** (§3.1.1) — the primary Phase 3 actuator. Zero-cross solid-state relay duty-cycling a resistive griddle element: no contact wear, so the time-proportioning window drops to 2–5 s for visibly smoother regulation than any mechanical-relay plug can deliver.
2. **Off-the-shelf smart plug** (Tasmota/ESPHome, mechanical relay) — quick-start alternative using hardware already in the HA/MQTT ecosystem. Same `MqttPlugActuator` interface, but limited to a 30 s window for relay life. Useful for bring-up before the SSR box exists and as the lowest-effort path for other users.
3. **Portable induction hob** — either relay-based (only if the hob tolerates power cycling and re-arms; most don't) or a hob with serial/IR control that can be reverse-engineered. Treat as a Phase 3.5 stretch; pick a specific RE-able model before committing.
4. **Gas knob actuation — explicitly out of scope for Phase 3.** On a gas or unactuated cooktop the device runs Stovetop Advisor configuration (§0.1) — the human is the actuator, driven by the escalating cue system (§3.5). Motorized gas control without flame-out detection and certified interlocks is a safety non-starter for a hobby-grade device; revisit only as Phase 4 research.

### 3.1.1 PanPilot SSR Box — hardware specification

A self-contained inline power controller: griddle plugs into the box, box plugs into the wall.

**Power path:**
- IEC C14 inlet (or captive 14 AWG cord) → 15 A fuse on hot → SSR → NEMA 5-15R outlet
- SSR: **zero-cross, 40 A-rated, name-brand only** (Crydom, Omron G3NA, Panasonic). Budget "Fotek SSR-40DA"-style clones are commonly 10 A dies in 40 A packaging and are prohibited. Load is 1200–1800 W resistive (10–15 A @ 120 V) — no inrush, no snubber needed
- **Heatsink is mandatory:** ~1.2 V forward drop ⇒ ~15 W dissipation at 12 A. Finned aluminum heatsink external to or vented from the enclosure, thermal compound, plus a **thermal fuse (~110 °C) on the heatsink** in series with the load or SSR control as a last-resort cutoff
- Enclosure: metal or flame-rated (V-0) plastic, no user-accessible live parts, strain relief both cords, earth continuity if metal

**Control side:**
- ESP32-C3 (or S3) module inside running **ESPHome**, driving the SSR input (3.3 V logic triggers standard AC SSRs directly)
- Publishes/subscribes on the same MQTT broker as PanPilot; presents as a standard switch/duty entity → PanPilot's existing `MqttPlugActuator` drives it unchanged
- **Hardware-local watchdog (implements S8):** ESPHome script forces SSR off if no duty command received for 15 s, and on any Wi-Fi/broker disconnect. PanPilot refuses to arm the box until the watchdog handshake acks
- Status LED: off / regulating (blinks with duty) / fault
- Optional: heatsink NTC reported over MQTT; current sense (e.g., shunt or SCT clamp) to confirm the element actually draws power — feeds interlock S7 with real load telemetry instead of inferred liveness

**Griddle prep:** set the appliance's own thermostat to **maximum, never bypassed.** It then serves as an independent, appliance-certified over-temp backstop (~450 °F). This matters because SSRs predominantly fail *shorted*: the defense stack for a stuck-ON SSR is (1) native griddle thermostat, (2) S5/S7 alarms telling the user to unplug, (3) thermal fuse, (4) line fuse.

## 3.2 Control Architecture

New module `core/control/`:

- **Actuator HAL:** `HeatActuator` interface — `setDuty(0..1)`, `emergencyOff()`, `isAlive()`, capability flags (binary vs proportional, **min cycle period**). Implementations: `MqttPlugActuator` (drives both the SSR box and generic smart plugs — the box just advertises a 2 s min cycle where a mechanical plug advertises 30 s), future `InductionSerialActuator`. In `ADVISORY` configuration the "actuator" is the human: the guidance/cue layer (§3.5) is the output stage and no `HeatActuator` is armed.
- **Control law:** time-proportional bang-bang → PID.
  - **Time-proportioning window scales to the actuator:** SSR box 2–5 s (default 3 s, zero contact wear, near-proportional regulation on griddle thermal mass); mechanical-relay plug 30 s (relay life). Duty = PID output. Phase-angle control is never needed — the plant time constant is minutes, so slow PWM is indistinguishable from proportional power.
  - PID with: feedforward from learned pan profile (expected duty to hold T from Learn Pan data), anti-windup, derivative on measurement, gain scheduling by temperature band (heat-up vs regulation).
  - Thermal lag dominates; the learned `LAG_MINUTES` from Phase 1.5 becomes the Smith-predictor-style dead-time estimate. Start with conservative gains + relay autotune option (Åström–Hägglund) run as a guided "Tune this appliance" wizard analogous to Learn Pan Mode.
  - Setpoint sources: preset target midpoint, or recipe program (Phase 4).
- **Modes:** `ADVISORY` (Phase 1.5 behavior, control disabled), `ASSIST` (device controls, human confirmed), `LOCKED_OUT` (interlock tripped).

## 3.3 Safety Interlock Specification (the heart of Phase 3 — implement before any control ships)

Control authority is a privilege the perception system must continuously earn. Hard rules, all fail-safe to **power off**:

| # | Interlock | Trip condition | Action |
|---|---|---|---|
| S1 | Confidence gate | ROI confidence < 60 for > 5 s | Duty → 0, alert "Control paused — check aim" |
| S2 | Pan presence | PAN REMOVED or ABSENT | Duty → 0 immediately |
| S3 | Obstruction | OBSTRUCTED > 10 s | Duty → 0 |
| S4 | Runaway / open-loop fault | Duty > 50 % for 60 s with rate ≤ 0 (boil-dry, aim on wrong object, actuator stuck) | Emergency off + repeating alarm |
| S5 | Hard max temp | pan > preset warn + 25 °F, or absolute 650 °F | Emergency off + alarm |
| S6 | Sensor fault | SENSOR FAULT (base spec §5) or frame gap > 3 s | Duty → 0 |
| S7 | Actuator heartbeat | `isAlive()` false / MQTT plug LWT offline / command unacked 10 s | Assume ON-stuck: alarm loudly, instruct manual unplug |
| S8 | Comms loss (MQTT actuator) | Broker or Wi-Fi drop | Plug must be configured with its **own** failsafe: `PulseTime`/watchdog so it turns itself off without PanPilot commands. PanPilot refuses to arm a plug that doesn't ack the watchdog config |
| S9 | Human override | Any touch on the big red STOP bar, physical button if fitted | Emergency off, requires explicit re-arm |
| S10 | Unattended timeout | No user interaction and no food events for 45 min under control | Duty → 0, alarm |
| S11 | Device self-heat | Internal die > 85 °C | Duty → 0 |

Additional requirements:
- Arming ceremony: entering ASSIST requires an explicit confirm screen naming the actuator ("Control ELECTRIC GRIDDLE via plug 'kitchen-plug-2'?") + the plug watchdog handshake (S8). No auto-arm on boot, ever.
- Persistent on-screen state whenever armed: actuator name, duty %, big STOP bar. Control state also mirrored to HA.
- Every interlock trip logged with cause to the session record.
- The interlock evaluator runs in `LogicTask` **before** the PID each tick and can only lower duty; the PID can never override an interlock.
- Unit tests: full interlock truth-table in `test_control` with scripted perception sequences; a PID that regulates a simulated first-order-plus-dead-time plant model within ±10 °F.

## 3.4 What Phase 3 unlocks in UX

- "Hold 350 °F" — walk away during preheat entirely; device chirps at READY with heat already stabilized
- Batch mode with control: after FOOD ADDED, controller boosts duty for recovery, then re-regulates — recovery time becomes a controlled variable instead of an alert
- Low-and-slow: hours-long holds (confit, tortilla warming, chocolate temper ~90 °F band on a hot plate) that are impractical with knob-fiddling

## 3.5 Attention & Cue Escalation System (`core/attention`)

The buzzer and display are the device's only way to reach a cook who is chopping, washing hands, or across the kitchen. Every cue in both configurations routes through one attention module with four escalation levels:

| Level | Screen | Buzzer | Used for |
|---|---|---|---|
| L0 — Passive | Status text / action bar color change | silent | trend updates, ETA ticks, HOLD |
| L1 — Notify | Action bar pulses once | single chirp | READY entered, food-added acknowledged, recovery complete |
| L2 — Act now | **Full-screen instruction card + backlight flash** (PWM strobe ~2 Hz, inverted colors) | double-beep, repeats every 5 s | TURN DOWN NOW, TURN TO MEDIUM, add-next-batch, flip cue |
| L3 — Alarm | Full-screen red, continuous strobe | urgent pattern, repeats until acknowledged or condition clears | TOO HOT, interlock trips S4/S5/S7, boil-dry suspicion |

Rules:
- **Instruction cards are verbs, kitchen-distance readable:** one line, e.g. `TURN TO MEDIUM`, `TURN DOWN NOW`, `FLIP`, `ADD BATCH 2`. Current temp small beneath. Tap anywhere = acknowledge.
- **Compliance verification (Stovetop Advisor only):** after a knob-change cue, the device *watches for the expected rate change* (e.g., "turn down" ⇒ rate should fall below +10 °F/min within 20 s). Complied → L1 confirmation chirp + card dismisses itself. Not complied → re-issue at L2; if temp then crosses the warn threshold, escalate to L3. This closes the human-in-the-loop control loop — the device knows whether you actually turned the knob.
- Escalation is rate-limited and per-event re-armed (base-spec hysteresis rules apply); mute suppresses L0–L2 audio but **never L3**.
- Quiet hours (Phase 2 setting) cap audio at L1 volume but never suppress L3 patterns.
- All levels mirrored to MQTT so HA can add lights/TTS reach (Phase 2 §2.2), but on-device buzzer+strobe must be fully sufficient with Wi-Fi off.
- Backlight strobe implemented via the existing `TFT_BL` PWM channel; must not exceed 3 Hz (photosensitivity) and must restore prior brightness on dismiss.

## 3.6 Phase 3 Milestones

- **M13 — Attention escalation system.** Accept: L0–L3 demonstrated on-device; Stovetop Advisor compliance verification works on a live burner (device detects the knob turn from the rate change, confirms with chirp, escalates when ignored).
- **M14 — SSR box build + actuator HAL + watchdog handshake.** Accept: §3.1.1 box assembled and load-tested at full griddle draw for 1 h with heatsink ≤ 70 °C; duty-cycles at 3 s window; killing Wi-Fi or the broker forces SSR off within 15 s without any PanPilot command.
- **M15 — Interlocks.** Accept: every S1–S11 row demonstrably trips on hardware (checklist doc); truth-table tests green.
- **M16 — Closed-loop hold.** Accept: electric griddle held at 350 ±15 °F for 30 min unattended via SSR box (time-proportional, pre-PID).
- **M17 — PID + autotune wizard.** Accept: hold tightens to ±8 °F; autotune completes on two different appliances (griddle + hot plate).
- **M18 — Controlled batch cooking.** Accept: 4 pancake batches, recovery-to-ready under control faster and more consistent than M6 advisory baseline (log comparison).

---

# PHASE 4 — Ultimate Goal: The Cooking Flight Director

The end state, per the parent spec's vision, is a **pan-temperature flight director** matured into a full **cook director**: it manages temperature (Phase 3), *time*, and *sequence*, and perceives enough about the food itself to adapt. The human is the actuator for physical steps; PanPilot is the brain and the clock.

## 4.1 Recipe Programs (deterministic core — no ML needed; **Phase 3-independent, see §0.2 Track B**)

A declarative, on-device cook-program format (JSON in LittleFS, editable in the web UI):

```json
{ "name": "Smash Burgers x4",
  "steps": [
    {"hold": 450, "until": "ready", "say": "Preheat"},
    {"cue": "Add 2 patties + smash", "expect": "food_added", "timeout": 120},
    {"timer": 90, "hold": 450, "say": "Searing side 1"},
    {"cue": "Flip + cheese", "expect": "food_added_or_touch"},
    {"timer": 60, "say": "Finishing"},
    {"cue": "Remove. Pan recovering for next batch", "expect": "recovery", "loop_to": 1, "loops": 2}
  ] }
```

Engine: step sequencer sitting above the guidance/control layer — each step's `hold` becomes a controller setpoint in Autopilot configuration **or** a knob-cue-plus-verify sequence in Advisor configuration (§0.1); the sequencer itself is identical. Each step arms expected perception events, runs timers, and cues the human (screen + buzzer pattern + HA TTS announcement via MQTT if configured). Steps reference `foodlib` entries (§2.7 / Appendix A) instead of hardcoding times — `{"cook": "smash_burger", "side": 1}` inherits that food's compensated timing, flip hints, and safety notes, so the database is authored once and reused everywhere. This alone delivers "repeatable cooking" — same pan, same program, same result — and requires no control hardware at all in Advisor configuration.

### 4.1.1 Recipe Creator (embedded web UI)

**Step-advancement semantics (normative, both configurations):** the sequencer is **event-gated, never clock-gated.** A step advances when perception confirms its condition; wall-clock time is only a fallback (escalation, never silent skip) or an explicit Timer/Rest step. Advisor vs Autopilot changes only *how* the condition gets satisfied, never whether the sequencer waits for it.

| Step type | Advances when | On timeout / failure |
|---|---|---|
| Hold / Preheat | Pan temp verifiably in band + stable (guidance READY). Autopilot: PID drives it there. Advisor: knob cue + compliance verification; sequencer waits indefinitely if the knob isn't turned | Escalate attention (L2→L3); never advance on an unmet setpoint |
| Cook (foodlib) | Doneness accumulator complete (temp-compensated; pauses on PAN REMOVED) | n/a — accumulator can only stretch, not skip |
| Prep (preplib) | Thermal ready signature (melted/equalized) or user tap; entry blocked above `maxAddTempF` until pan cools | Fall back to tap-confirm at low confidence |
| Cue | Its `expect` perception event (food_added, recovery, …) or user tap | Re-cue at higher level, then pause "waiting for you" — **never auto-advance** |
| Timer / Rest | Wall clock (the only clock-gated steps, and explicitly so) | n/a |
| *(program-wide)* | — | PAN REMOVED pauses the program; any Autopilot interlock trip pauses sequencer + cuts duty; resume only from re-armed verified state |

Authoring recipe JSON by hand is fine for the developer and hostile to everyone else. The Recipe Creator is a page in the Phase 2 embedded web UI (§2.2) that builds, validates, simulates, and saves programs. It is the primary authoring surface; **on-device editing is deliberately limited to parameter tweaks** (nudging a time or temp on an existing program) — full authoring is web-only, keeping the LVGL UI simple.

**Serving constraints (it lives on an ESP32):**
- Single-page vanilla JS + CSS, gzipped into LittleFS with the rest of the web assets; budget **≤ 60 KB gzipped** for the whole page
- Zero CDN/internet dependencies — must work on an isolated network, consistent with local-first (§2.2)
- The browser is presentation only; **the firmware validator is the single source of truth** (browser mirrors rules for instant feedback, device re-validates on save)

**REST API (extends §2.2 scaffolding):**
```
GET    /api/foodlib                     merged seed + user overrides (Appendix A entries)
GET    /api/programs                    list (id, name, est. total time, schema ver)
GET    /api/programs/{id}               full JSON
PUT    /api/programs/{id}               save (server-side validate; 422 with reasons on failure)
DELETE /api/programs/{id}
POST   /api/programs/validate           validation verdict without saving
POST   /api/programs/{id}/dryrun        simulated timeline (see below)
POST   /api/programs/{id}/stage         queue program on device — starting still requires
                                        an on-device confirmation tap, always
```
The stage-vs-start split is deliberate: no cook ever begins from a browser. Same philosophy as the §3.3 arming ceremony — remote surfaces propose, the touchscreen disposes.

**Editor features:**
- **Step palette:** Preheat/Hold (temp), Cook (foodlib entry + side), **Prep (preplib ingredient — see below)**, Cue (text + expected perception event + timeout), Timer, Rest, Loop (back-reference + count). Drag to reorder; parameters edited inline.
- **"New from food" wizard — the 90 % path:** pick a foodlib entry and a batch count → generator emits preheat → add-cue → per-side cook steps with flip cues → remove (+ safety note if applicable) → recovery-loop for remaining batches. Most users never touch the step palette.
- **Foodlib picker** with search; Cook steps live-display the inherited compensated times, temps, flip hints, and safety notes from the selected entry so authors see what they're getting.
- **Dry-run timeline:** horizontal timeline of the simulated program — setpoint band, cue markers, per-step durations, estimated total. In Advisor annotation mode it also marks where knob cues will fire, so the author sees the difference between running it on the stove vs. the SSR griddle before ever cooking it.
- **Import/export:** `.panrecipe` JSON files for sharing/backup; schema-version stamped.

**Prep steps & the ingredient advice engine (`core/preplib`):**

Real recipes don't start with food — they start with *"add butter, let it melt and foam"* or *"add a tablespoon of oil, heat until shimmering."* Prep steps make these first-class, backed by a small ingredient table (**preplib, Appendix A.1**) of fats and liquids with per-ingredient add windows, smoke points, and readiness criteria.

*Runtime behavior of a Prep step:*
- Cues `ADD BUTTER` (L2) when pan temp enters the ingredient's add window; if the pan is **above** the ingredient's `maxAddTempF`, the cue is preceded by `PAN TOO HOT FOR BUTTER — cooling to 300°F` and the sequencer waits (Autopilot cuts duty; Advisor cues a knob turn). Butter hitting a 425 °F pan burns before you can reach for the spatula — the device prevents that class of error entirely.
- **Melt/ready detection is thermally verifiable:** a butter pat appears as a cold blob inside the pan ROI (same signature as FOOD ADDED), then spreads and re-equalizes as it melts. Ready criterion = ROI temperature spread collapsing back to baseline + temp recovery. Oil similarly shows a dip then equalization; "oil at pan temp" is the shimmer-adjacent proxy. Auto-advance on detection, with tap-to-confirm always available and automatic fallback to tap-only when confidence < 50.
- Readiness hint from the table shows as the sub-line: *"foaming subsides = ready,"* *"shimmers, flows like water."*
- **Fat state tracking:** once a Prep step completes, the session records which fat is in the pan, and the overheat warning clamps to `min(preset warn, fat smoke point − 25 °F)` until pan-wash/removal or session end. The device knowing there's EVOO in the pan and warning at 375 °F instead of the sear preset's 600 °F is a quality *and* safety feature no timer app can replicate. Exposed to Thermometer/Target modes too, not just programs — a "what's in the pan?" quick-pick on the cooking screen.

*Authoring-time advice (the "advise" in Recipe Creator):*
- **Fat-vs-temperature compatibility:** if any downstream `hold`/Cook step exceeds the chosen fat's smoke point − margin, the editor flags it inline with suggested swaps: *"Butter smokes at ~300 °F; your sear holds 500 °F. Use avocado oil or ghee, or add butter as a finishing step after the sear."* Suggestion list is generated from preplib entries whose smoke points clear the program's max temp.
- **Order sanity:** Prep-fat steps placed after food is already down get a soft warning; butter-as-finisher (post-Cook baste step) is recognized as valid and not flagged.
- **Two-stage fat idiom supported:** the wizard knows the classic pattern — high-smoke oil for the sear, butter added at the end for basting/flavor — and offers it as a toggle when the program's max temp exceeds butter's ceiling.
- Wizard integration: "New from food" gains a fat picker (defaulted per food — butter for eggs/pancakes/grilled cheese, neutral oil for smash burgers, avocado/ghee for sear) and inserts the Prep step at the technique-correct point: fats go in *during* preheat at their add window, not after the pan hits final temp.

**Validation rules (enforced device-side, mirrored in browser):**
- Every `hold` ≤ absolute cap (650 °F, interlock S5's ceiling) and flagged if above the referenced food's warn threshold
- **Prep-fat compatibility:** any hold/Cook temp exceeding an in-pan fat's smoke point − 25 °F is a blocking error unless the author explicitly acknowledges ("I know — browning butter on purpose"), which stamps the program and softens the runtime clamp to the smoke point itself
- Loop back-references must point to an earlier step; loop counts bounded (≤ 20)
- Every Cue step needs an expected event and/or a timeout — no steps that can hang a cook forever
- Cook steps referencing foods with `safeInternalF ≠ 0` get the verify-internal note welded to their remove cue; **the editor cannot remove it**
- Program size ≤ 8 KB, ≤ 40 steps; device stores ≤ 32 programs in `/programs/*.json` (LittleFS)
- Unknown foodlib references rejected at save (catch renamed/deleted foods)

**Track B milestones (fills in the M19+ range from §0):**
- **M19 — Recipe sequencer engine (Advisor mode).** Accept: a hand-authored smash-burger ×4 JSON runs end-to-end on the stovetop; all cues fire through the attention system; knob-cue compliance verification works inside a program; **a butter Prep step auto-advances on thermally detected melt, and cueing butter onto a deliberately overheated pan triggers the too-hot-for-butter hold.**
- **M20 — Recipe Creator web UI.** Accept: wizard generates the same smash-burger ×4 program from the foodlib entry in under a minute; dry-run total time within ±15 % of an actual cook; validator rejects a 700 °F hold, a poultry program with the safety note stripped, **and a butter-then-500°F-sear program (offering avocado oil/ghee swaps and the butter-as-finisher pattern)**; page loads with Wi-Fi isolated from the internet.
- **M21 — Autopilot recipes (Track A + B merge).** Accept: the identical program file runs closed-loop on the SSR griddle with zero edits; the §4.4 pancake end-state scenario is the acceptance test.

## 4.2 Food-Aware Perception (research track — the 3090s earn their keep)

The thermal matrix sees more than pan temperature. Research items, roughly in order of feasibility:

1. **Food-region segmentation:** food pixels are the cold blob(s) *inside* the pan blob. Track per-item surface temperature and count ("2 patties detected").
2. **Doneness proxies from thermal dynamics:** surface-temp trajectory of a pancake predicts flip time (bubble/set transition shows as a rate change); patty surface approaching pan temp ≈ crust formed. Collect labeled traces via session logging (Phase 2 infra) → train small temporal models offline on the EPYC/3090 rig → distill to thresholds or a tiny TFLite-Micro model on the S3. The §2.7 doneness accumulator is the prior these models refine — and the "Under/Perfect/Over" feedback taps are the labels.
3. **State classification:** empty / oiled / food-loaded / smoking-risk (rapid localized hotspots on an empty pan at high temp) from frame statistics.
4. Cue upgrade: "Flip in ~20 s" instead of fixed timers — the flight director starts flying the food, not just the pan.

Explicit non-goals even in Phase 4: visible-light camera (privacy + scope), cloud inference, internet-required anything.

## 4.3 Whole-Kitchen Integration

- Multiple sensor heads / multiple PanPilots federated over MQTT; one "kitchen state" in HA
- Oven/probe fusion: ingest a BLE meat-probe or HA temperature entity so programs can span pan + oven ("sear then finish at internal 130 °F")
- Voice: HA Assist / TTS cues so the screen becomes optional mid-cook
- Gas revisited *only* here: flame-presence sensing (thermal matrix can actually see the flame region below/around the pan on many ranges — characterize this) + certified actuator review before any knob motorization

## 4.4 Product End-State Definition

PanPilot is done, as a vision, when this session is routine:

> User taps "Pancakes ×12," walks away. Device preheats and holds the griddle, chirps "pour 4," watches them cook, says "flip" per-pancake as each is ready, manages recovery between batches, announces "done — 12 pancakes, 14 minutes," logs the session, and powers the griddle down. Total user attention: pouring and flipping on cue.

Success criteria: a first-time cook produces results indistinguishable from the household's best cook for the programmed recipes; zero interlock-bypass incidents; the device is trusted enough to be left armed.

## 4.5 Landscape Note (prior art & positioning)

Adjacent products to study before Phase 3/4 investment — both for design lessons and freedom-to-operate: Hestan Cue (pan sensor + induction closed loop, Bluetooth-paired), Breville/PolyScience Control Freak (probe-through-glass induction PID), Combustion Inc. Predictive Thermometer (multi-sensor prediction UX), Meater (wireless probe + ETA framing), plus consumer thermal-camera cooking patents generally. PanPilot's differentiated stack is **contactless thermal-matrix perception + confidence-gated control authority + retrofit of dumb appliances** — no instrumented pan, no proprietary cooktop. Worth a proper prior-art pass before Phase 3 hardware spend; the interlock architecture (perception confidence as a control-arming signal) may itself be protectable.

---

## 5. Architecture Evolution Summary

```
Phase 1.5:  Sensor → Perception → Guidance → Human → Stove
Phase 2:    + Wi-Fi/HA/web mirror, battery, logging, multi-pan

Phase 3 (both configurations, one firmware):
  Advisor:    Sensor → Perception → Guidance → Attention cues → Human → Knob
                                        ↑ compliance verified from rate change
  Autopilot:  Sensor → Perception → Interlocks → PID → SSR box → Griddle
                                        ↑ Human STOP always wins

Phase 4:    Recipe Engine → (setpoints OR knob cues) → Phase-3 loop
            Food perception feeds Recipe Engine
```

Every phase strictly adds layers; nothing in M0–M6 gets rewritten. The base spec's rule that `core/` is hardware-free and unit-tested is what makes M13–M18's control code testable against simulated plants before it ever touches a live griddle.

## 6. Risk Register (top items)

| Risk | Phase | Mitigation |
|---|---|---|
| MLX90640 accuracy drift with grease film on sensor | 2+ | Replaceable shield ring, "clean sensor" reminder from confidence trending, per-profile offset recal |
| SSR fails shorted (stuck ON) | 3 | Defense stack §3.1.1: griddle's native thermostat at max (never bypassed) + S5/S7 alarms + heatsink thermal fuse + line fuse |
| Counterfeit/underrated SSR overheats | 3 | Name-brand 40 A part only, mandatory heatsink, 1 h full-load thermal test in M14, optional NTC telemetry |
| Mechanical relay wear (smart-plug path) | 3 | 30 s min window enforced by capability flag; cycle counter in logs; SSR box is the primary path |
| Stuck-ON / unreachable actuator | 3 | S7/S8: actuator-side watchdog is mandatory, not optional — box self-offs in 15 s without commands |
| Cook ignores knob cues (Advisor config) | 3 | §3.5 compliance verification + escalation to L3 before warn threshold is crossed |
| Liability of "control" framing | 3 | Prominent supervised-use language, unattended timeout S10, never market gas control |
| Doneness ML overpromise | 4 | Ship deterministic recipe programs first; ML cues labeled "beta," always paired with timers |

---

# APPENDIX A — Seed Food Database (`foodlib_seed.h`)

Referenced by §2.7 and §4.1. Implementer: place at `src/core/foodlib/foodlib_seed.h` verbatim. Times/temps are authored starting values pending human review (rows marked `[REVIEW]` have lower authoring confidence); the `safeInternalF` field is safety-critical and must never be zeroed to reduce UI noise.

```c
// =============================================================================
// foodlib_seed.h — PanPilot Cook-Time & Temperature Seed Database
// Implements spec §2.7 (panpilot-phase2-to-ultimate-spec.md, v1.1)
//
// UNITS / CONVENTIONS
//   - All temperatures °F (PAN SURFACE temperature, not air, not internal)
//   - All times in SECONDS, authored at refTempF
//   - sideSec[] entries beyond `sides` count must be 0
//   - sides: 1 = no flip, 2 = flip once, 3–4 = multi-turn (sausages)
//   - tempCompPctPer25F: % change in cook time per 25 °F pan deviation from
//     refTempF (negative = hotter pan cooks faster). Runtime clamps total
//     compensation to 0.7x–1.3x per spec.
//   - safeInternalF: USDA minimum internal temps — 165 poultry, 160 ground
//     meats, 145 whole-muscle pork/beef/fish (with rest). 0 = visual doneness,
//     no internal-temp claim. Any nonzero value forces the "verify internal
//     temp" line on the REMOVE cue. THIS FIELD IS SAFETY-CRITICAL; do not
//     zero it to reduce UI noise.
//
// DATA PROVENANCE
//   Times/temps are consensus ranges from mainstream published cooking
//   references and griddle/skillet charts, tuned to the midpoint of each
//   preset's target band. They are STARTING VALUES: the ±8% Under/Perfect/
//   Over feedback loop (spec §2.7) is expected to personalize them.
//   >>> HUMAN REVIEW REQUIRED before first release — James: eyeball every
//   >>> row, especially steak and poultry. Marked [REVIEW] where authoring
//   >>> confidence is lower (thickness-sensitive items).
//
// This table is compiled into flash. User overrides & custom foods live in
// LittleFS JSON (spec §2.7) and shadow entries by (name, variant) key.
// =============================================================================

#pragma once
#include <stdint.h>

#define FOODLIB_MAX_SIDES 4
#define FOODLIB_SCHEMA_VERSION 1

typedef struct {
    const char* name;
    const char* variant;
    uint16_t    panTargetF_lo;
    uint16_t    panTargetF_hi;
    uint16_t    sideSec[FOODLIB_MAX_SIDES];
    uint8_t     sides;
    uint16_t    refTempF;
    int8_t      tempCompPctPer25F;
    uint16_t    restSec;
    uint16_t    safeInternalF;
    const char* flipHint;
} FoodEntry;

static const FoodEntry FOODLIB_SEED[] = {

// ---------------------------------------------------------------- Breakfast
{ "Pancakes", "4-inch, standard batter",
  350, 375, {150,  90, 0, 0}, 2, 365, -12,   0,   0,
  "Flip when bubbles pop and stay open, edges look set" },

{ "Pancakes", "large / 6-inch",
  350, 375, {195, 120, 0, 0}, 2, 365, -12,   0,   0,
  "Flip when bubbles pop and stay open, edges look set" },

{ "French toast", "standard sliced bread",
  325, 350, {180, 150, 0, 0}, 2, 340, -12,   0,   0,
  "Flip when underside is deep golden" },

{ "Eggs", "sunny side up",
  275, 300, {180,   0, 0, 0}, 1, 285, -15,   0,   0,
  "Done when whites are fully set, yolk still soft — no flip" },

{ "Eggs", "over easy",
  275, 300, {120,  30, 0, 0}, 2, 285, -15,   0,   0,
  "Flip gently once whites are set; brief second side" },

{ "Eggs", "scrambled, soft",
  250, 300, {150,   0, 0, 0}, 1, 275, -15,   0,   0,
  "Stir/fold continuously; pull while slightly glossy — carryover finishes" },

{ "Omelette", "3-egg, folded",
  275, 325, {105,  30, 0, 0}, 2, 300, -15,   0,   0,
  "\"Flip\" = add filling and fold once top is nearly set" },

{ "Bacon", "regular cut",
  325, 350, {240, 180, 0, 0}, 2, 335, -12,   0,   0,
  "Flip when edges curl and underside is browned; go by look, not clock" },

{ "Bacon", "thick cut",
  325, 350, {300, 240, 0, 0}, 2, 335, -12,   0,   0,
  "Flip when edges curl and underside is browned" },

{ "Hash browns", "shredded, 1/2-inch layer",
  365, 400, {300, 240, 0, 0}, 2, 375, -10,   0,   0,
  "Flip once bottom crust is deep golden and holds together" },

{ "Sausage links", "fresh pork breakfast links",
  325, 350, {180, 180, 180, 180}, 4, 335, -10,   0, 160,
  "Quarter-turn each cue for even browning" },

// ---------------------------------------------------------------- Burgers
{ "Smash burger", "2 oz ball, smashed thin",
  450, 500, { 90,  45, 0, 0}, 2, 475, -10,   0, 160,
  "Scrape-flip when crust is deep brown; cheese on side 2" },

{ "Burger", "1/3 lb, 3/4-inch patty, medium",
  375, 425, {240, 180, 0, 0}, 2, 400, -12, 120, 160,
  "Flip once, don't press — juices stay in" },

// ---------------------------------------------------------------- Steak  [REVIEW]
// Whole-muscle beef: USDA 145 with 3-min rest. Times are strongly
// thickness-dependent; variants pin thickness. Pull temps for target
// doneness run below 145 by preference — device still shows the 145 note.
{ "Steak", "1-inch, rare",
  450, 550, {135, 120, 0, 0}, 2, 500, -10, 300, 145,
  "Flip once when crust releases freely; rest is mandatory" },

{ "Steak", "1-inch, medium rare",
  450, 550, {195, 165, 0, 0}, 2, 500, -10, 300, 145,
  "Flip once when crust releases freely; rest is mandatory" },

{ "Steak", "1-inch, medium",
  450, 550, {240, 210, 0, 0}, 2, 500, -10, 300, 145,
  "Flip once when crust releases freely; rest is mandatory" },

{ "Steak", "1.5-inch, medium rare  [REVIEW]",
  450, 550, {270, 240, 0, 0}, 2, 500, -10, 420, 145,
  "Consider finishing thick cuts with a probe — surface timing is a guide" },

// ---------------------------------------------------------------- Poultry
{ "Chicken breast", "butterflied / ~1/2-inch",
  350, 400, {240, 210, 0, 0}, 2, 375, -10, 180, 165,
  "Flip when underside is golden; 165°F internal is non-negotiable" },

{ "Chicken thigh", "boneless skinless",
  350, 400, {300, 240, 0, 0}, 2, 375, -10, 180, 165,
  "Thighs forgive overshoot; verify 165°F internal" },

// ---------------------------------------------------------------- Pork
{ "Pork chop", "3/4-inch boneless",
  400, 450, {210, 180, 0, 0}, 2, 425, -10, 180, 145,
  "Flip once; pull at 145°F internal + rest for juicy, not gray" },

// ---------------------------------------------------------------- Seafood
{ "Salmon", "1-inch fillet, skin-on",
  400, 450, {240, 120, 0, 0}, 2, 425, -12,  60, 145,
  "Start skin-down 80% of the cook; flip for a short finish" },

{ "White fish", "cod/tilapia, 3/4-inch",
  375, 425, {180, 120, 0, 0}, 2, 400, -12,   0, 145,
  "Flip carefully once underside releases; flakes when done" },

{ "Shrimp", "large, peeled",
  400, 450, { 90,  60, 0, 0}, 2, 425, -15,   0,   0,
  "Done at opaque and pink with a C-curl — overcooked at an O" },

// ---------------------------------------------------------------- Flattops & melts
{ "Grilled cheese", "standard, buttered",
  300, 325, {150, 120, 0, 0}, 2, 315, -15,   0,   0,
  "Low and slow: bread golden AND cheese melted, not one or the other" },

{ "Quesadilla", "single tortilla, folded",
  350, 375, {120,  90, 0, 0}, 2, 365, -12,   0,   0,
  "Flip when underside is blistered golden" },

{ "Tortillas", "warming, corn or flour",
  400, 450, { 30,  30, 0, 0}, 2, 425, -15,   0,   0,
  "Puffing is the done signal" },

// ---------------------------------------------------------------- Vegetarian
{ "Tofu", "extra-firm slabs, pressed",
  375, 425, {240, 180, 0, 0}, 2, 400, -10,   0,   0,
  "Don't touch until it releases on its own — that's the crust forming" },

{ "Smash potatoes", "par-boiled, smashed  [REVIEW]",
  375, 425, {300, 240, 0, 0}, 2, 400, -10,   0,   0,
  "Flip when edges are deep golden and crisp" },

};

#define FOODLIB_SEED_COUNT (sizeof(FOODLIB_SEED)/sizeof(FOODLIB_SEED[0]))

// -----------------------------------------------------------------------------
// Implementation notes for the timer engine (core/foodtimer):
//  - Doneness accumulator per spec §2.7: progress += dt * k(panTemp),
//    k = 1 + tempCompPctPer25F/100 * ((refTempF - panTempF)/25), clamp 0.7..1.3.
//    (Sign: pan hotter than ref -> k > 1 -> finishes sooner.)
//  - restSec runs after the REMOVE cue at attention level L1.
//  - safeInternalF != 0 appends: "Surface timing only — verify <T>°F internal"
//    to the REMOVE card. Never suppress.
//  - Feedback nudges (±8%) apply per (name, variant) in LittleFS overrides,
//    multiplicative on all sideSec, bounded to 0.6x–1.5x of seed lifetime.
// -----------------------------------------------------------------------------
```

---

# APPENDIX A.1 — Prep Ingredient Table (`preplib_seed.h`)

Referenced by §4.1.1. Smoke points are conservative consensus values for consumer-grade products; refined variants vary by brand. `maxAddTempF` is the pan temp above which the ADD cue is held (with cooldown) — for butter this is well below its smoke point because milk solids scorch on contact. All temps °F pan-surface.

```c
#pragma once
#include <stdint.h>

typedef enum { READY_MELTED, READY_EQUALIZED, READY_IMMEDIATE } PrepReady;

typedef struct {
    const char* name;
    uint16_t addTempF_lo;      // ideal add window (cue fires entering this band)
    uint16_t addTempF_hi;
    uint16_t maxAddTempF;      // above this: hold cue, cool pan first
    uint16_t smokePointF;      // runtime overheat clamp = this - 25
    PrepReady readyCriterion;  // thermal auto-advance signature
    const char* readyHint;     // sub-line on the cue card
} PrepEntry;

static const PrepEntry PREPLIB_SEED[] = {
// name                addLo addHi maxAdd smoke  ready              hint
{ "Butter",             225,  300,  325,  300, READY_MELTED,    "Foaming subsides = ready; solids brown fast past this" },
{ "Ghee / clarified",   250,  400,  450,  465, READY_MELTED,    "Melts clear; safe well past butter temps" },
{ "Olive oil (EVOO)",   250,  350,  375,  375, READY_EQUALIZED, "Shimmers and flows like water when ready" },
{ "Olive oil (light)",  250,  400,  440,  465, READY_EQUALIZED, "Shimmers when ready" },
{ "Canola/vegetable",   250,  400,  425,  400, READY_EQUALIZED, "Shimmers when ready; wisps of smoke = too far" },
{ "Avocado oil",        250,  475,  500,  500, READY_EQUALIZED, "The high-heat sear oil; shimmers when ready" },
{ "Peanut oil",         250,  425,  450,  450, READY_EQUALIZED, "Shimmers when ready" },
{ "Bacon fat",          225,  325,  350,  370, READY_MELTED,    "Melted and glossy = ready" },
{ "Coconut (refined)",  250,  375,  400,  400, READY_MELTED,    "Melts clear" },
{ "Water (steam/pot-stickers)", 212, 450, 550, 0, READY_IMMEDIATE, "Expect a loud sizzle and temp drop - lid on" },
};

#define PREPLIB_SEED_COUNT (sizeof(PREPLIB_SEED)/sizeof(PREPLIB_SEED[0]))

// Notes:
//  - smokePointF == 0 means no smoke clamp (water). Water instead suppresses
//    the recovery-rate alarm briefly: a steam drop is expected, not a fault.
//  - Runtime clamp while fat is in pan: warn = min(preset_warn, smokePointF-25),
//    unless the program carries the authors "browning on purpose" stamp
//    (then clamp = smokePointF).
//  - [REVIEW] all values before release, same policy as Appendix A.
```
