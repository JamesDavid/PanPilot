// =============================================================================
// app_config.h — every tunable constant, one place (base spec §13).
//
// Rule: each constant states its UNITS and the SPEC SECTION it came from. No
// magic numbers scattered in the code. Thresholds here are the defaults; those
// the user can edit at runtime are persisted to NVS (base spec §10) and this
// value is only the factory default.
//
// This file grows per milestone. At M0 it holds only what board bring-up needs;
// §6–§9 perception/guidance constants land with M1–M4.
// =============================================================================
#pragma once

// ---- Firmware identity ------------------------------------------------------
// Full semver+githash is injected by the CI/PIO build script; this is a
// fallback shown if the build define is absent (base spec §13).
#ifndef PANPILOT_FW_VERSION
  #define PANPILOT_FW_VERSION "0.0.0-dev"
#endif

// ---- Display / backlight (base spec §8, §9) --------------------------------
#define BACKLIGHT_PWM_FREQ_HZ      5000   // Hz — LEDC backlight PWM
#define BACKLIGHT_PWM_BITS         8      // bits — LEDC resolution (0..255)
#define BACKLIGHT_ACTIVE_BRIGHT    255    // 0..255 — ACTIVE state (base spec §8)
#define BACKLIGHT_IDLE_BRIGHT      26     // 0..255 — IDLE ~10% (base spec §8)

// ---- Buzzer (base spec §2.3) -----------------------------------------------
#define BUZZER_PWM_FREQ_HZ         2731   // Hz — piezo resonant-ish default
#define BUZZER_PWM_BITS            8
#define BUZZER_CHIRP_MS            80     // ms — short chirp unit

// ---- I2C bus (base spec §2.2) ----------------------------------------------
#define I2C_FREQ_HZ                400000 // Hz — 400 kHz min for MLX frames;
                                          // raise toward 1 MHz if wiring allows.

// ---- Task cadence (base spec §4) -------------------------------------------
#define UI_TICK_MS                 5      // ms — lv_timer_handler period
#define LOGIC_TICK_HZ              4      // Hz — LogicTask
#define SENSOR_SUBPAGE_HZ          8      // Hz — MLX90640 subpage reads

// NOTE (M1): on boards with CAP_I2C_SHARED_TOUCH_SENSOR, touch (UI core) and
// MLX90640 frames (sensor core) share one I2C bus — guard bus access with a
// mutex so a 1.7 KB frame read never collides with a touch poll (base spec §4).

// ---- Sensor / MLX90640 (base spec §5) --------------------------------------
#define MLX_REFRESH_HZ             8      // Hz — chess mode, 8 Hz subpages
#define EMISSIVITY_DEFAULT         0.95f  // seasoned iron/steel/nonstick (§2.2)
#define SENSOR_MIN_VALID_C         -40.0f // reject pixels outside this...
#define SENSOR_MAX_VALID_C         600.0f // ...range (§5)
#define SENSOR_BAD_FRAMES_FAULT    10     // consecutive bad frames -> SENSOR FAULT
#define SENSOR_DIE_WARN_C          85.0f  // die temp warning (§5)
// Frame orientation. The MLX90640 pixel order is defined looking FROM the
// sensor AT the scene, so rendered camera-style it mirrors; flip applied at
// the HAL so analysis/ROI/web all agree. X flip bench-confirmed 2026-07-12
// (D55 breakout, first live frames); Y flip reserved for an upside-down
// enclosure mount.
#define SENSOR_FLIP_X              1      // mirror columns (0/1)
#define SENSOR_FLIP_Y              0      // mirror rows (0/1)

// ---- Frame analysis / ROI (base spec §6) -----------------------------------
#define PAN_DELTA_C                10.0f  // px > bg+this = candidate (§6.1)
#define PAN_ABS_HOT_C              60.0f  // ...or absolute hot during cooking
#define MIN_PAN_PIXELS             6      // min blob area (§6.1)
#define COOLING_ABS_DELTA_C        8.0f   // ambient+this inside last ROI = still there
#define ROI_PERCENTILE             75     // interior percentile -> panTemp (§6.2)
#define CENTROID_LP_ALPHA          0.35f  // centroid low-pass (§6.3 anti-jitter)
#define PRESENCE_ABSENT_MS         3000   // no blob this long -> ABSENT (§6.3)
#define CONFIDENCE_UNCERTAIN       30     // below this -> UNCERTAIN (§6.3)
#define STAINLESS_SPREAD_C         25.0f  // interior spread above this + low temp
#define STAINLESS_CONF_CAP         50     // ...caps confidence, sets hint (§6.3)
#define STAINLESS_TOOHOT_MIN_F     300    // below this, a stainless TOO_HOT is a
                                          // reflective misread — suppress it (§7.5)
#define MOVED_JUMP_PX              4       // centroid jump -> PAN MOVED (§6.4)
#define FOOD_DROP_C                15.0f  // panTemp drop -> FOOD ADDED (§6.4)
// Obstruction (§6.4): a hand / cool utensil over a previously-hot pan removes
// the hot pixels but leaves a warm (non-ambient, non-hot) mass at the last ROI.
#define OBSTRUCT_WARM_DELTA_C      5.0f   // above ambient to count as "covered"
#define OBSTRUCT_COVER_MAX_C       45.0f  // above this it's a warm pan, not a cover
#define OBSTRUCT_WAS_HOT_C         50.0f  // the tracked pan must have been this hot
#define OBSTRUCT_COVER_FRAC        0.6f   // this fraction of the ROI window covered

// ---- Thermal model: smoothing / rate / trend (base spec §7.1, §6.2) --------
#define SMOOTH_ALPHA               0.12f  // exp-smoothing, ~2 s tau at 4 Hz
#define RATE_WINDOW_MS             10000  // least-squares slope window (§7.1)
#define RATE_MIN_SPAN_MS           5000   // need >=5 s present data for a rate
// Trend thresholds, °F/min (base spec §6.2): stable <5, slow 5-20, fast >=20.
#define TREND_STABLE_FMIN          5.0f
#define TREND_SLOW_FMIN            20.0f

// ---- Guidance / Target Assist (base spec §7.1, §7.2) -----------------------
#define LAG_MINUTES_DEFAULT        0.5f   // thermal lag for overshoot (§7.1);
                                          // replaced by learned lag at M6.
#define ETA_MIN_RATE_FMIN          5.0f   // need this rate toward target for ETA
#define ETA_CLAMP_SEC              1200   // cap displayed ETA at 20 min (§7.1)
#define ETA_MIN_CONFIDENCE         50     // ETA needs confidence >= this (§7.1)
#define TURN_DOWN_SOON_BAND_F      15.0f  // within this below lo + rising => SOON
#define RATE_MEDIUM_FMIN           20.0f  // medium/fast rise (§6.2)
#define READY_REARM_F              10.0f  // must leave range by this to re-chime
#define TOO_HOT_REPEAT_MS          10000  // TOO_HOT alarm cadence (§7.2)
#define GUIDANCE_HYSTERESIS_TICKS  2      // ticks to confirm a crossing (§7.2)
#define ABS_MAX_TEMP_F             650.0f // hard ceiling (interlock S5 / §7.2)
// Generic target defaults (base spec §7.3; presets arrive at M4).
#define TARGET_DEFAULT_CENTER_F    350
#define TARGET_READY_WINDOW_F      10     // +/- around center (ready band)
#define TARGET_WARN_OVER_F         100    // warn = center + this (clamped 650)
#define TARGET_STEP_F              5       // adjuster step

// ---- Recovery Monitor (base spec §7.4) -------------------------------------
#define RECOVERY_SLOW_SEC          180     // projected recovery > 3 min -> SLOW
#define RECOVERY_FAST_FMIN         60.0f   // recovering faster than this -> FAST
#define FOOD_ADDED_WINDOW_MS       3000    // drop must occur within this (§6.4)

// ---- Learn Pan Mode (base spec §7 Phase 1.5) -------------------------------
#define LEARN_DURATION_MS          30000   // record heating for 30 s

// ---- Recipe prep / fats (roadmap §4.1.1) -----------------------------------
#define PREP_MELT_DWELL_MS         20000   // butter/solid fats: this long in the
                                           // add window = melted & ready
#define PREP_EQUALIZE_RATE_FMIN    8.0f    // oils: |rate| below this in the add
                                           // window = pan equalized (shimmering)

// ---- Closed-loop control + interlocks (roadmap spec §3.2, §3.3) ------------
#define IL_CONF_MIN                60      // S1: ROI confidence floor
#define IL_CONF_MS                 5000    // S1: for this long
#define IL_OBSTRUCT_MS             10000   // S3: obstruction this long
#define IL_RUNAWAY_DUTY            0.50f   // S4: duty above this...
#define IL_RUNAWAY_MS              60000   // S4: ...for this long with rate<=0
#define IL_HARD_MAX_OVER_F         25.0f   // S5: warn + this...
#define IL_HARD_MAX_ABS_F          650.0f  // S5: ...or absolute
#define IL_FRAME_GAP_MS            3000    // S6: no frame this long
#define IL_ACT_UNACK_MS            10000   // S7: actuator command unacked
#define IL_UNATTENDED_MS           2700000 // S10: 45 min no interaction
#define IL_DIE_MAX_C               85.0f   // S11: device die temp
#define CTRL_WINDOW_SSR_MS         3000    // SSR time-proportioning window
#define CTRL_WINDOW_PLUG_MS        30000   // mechanical relay window (relay life)
#define CTRL_ACT_WATCHDOG_MS       15000   // actuator self-off if no command

// ---- Attention & cue escalation (roadmap spec §3.5) ------------------------
#define ATTN_L2_REPEAT_MS          5000    // L2 re-beep cadence
#define ATTN_L3_REPEAT_MS          2000    // L3 alarm cadence
#define ATTN_STROBE_MAX_HZ         3       // photosensitivity cap
#define COMPLY_WINDOW_MS           20000   // watch for knob-turn effect this long
#define COMPLY_RATE_FMIN           10.0f   // "turned down" => rate falls below this
#define OVERLAY_READY_MS           6000    // ms: READY takeover auto-dismisses to
                                           // the action bar (a pan HOLDING at temp
                                           // is READY forever — bench 2026-07-11)
#define TOUCH_SCROLL_GAIN          -1.6f   // scroll travel multiplier (hal touch
                                           // reader): |g|>1 = faster than the
                                           // finger, negative = reversed swipe
                                           // (bench 2026-07-12 request). Taps
                                           // are unaffected (zero travel).

// ---- Battery / power subsystem (roadmap spec §2.1) -------------------------
#define BATTERY_LOW_PCT            15      // low-battery event threshold
#define BATTERY_CRITICAL_PCT      5       // critical threshold
#define BATTERY_REARM_PCT         3       // rise this far above to re-arm event
#define BACKLIGHT_BATTERY_BRIGHT  178     // ~70% on battery (roadmap §2.1)
#define MAX17048_I2C_ADDR         0x36    // fuel gauge (shared I2C bus)

// ---- Power / idle / sessions (base spec §8) --------------------------------
#define IDLE_TIMEOUT_MS            600000  // 10 min no touch + cool -> IDLE
#define WAKE_TOUCH_MS              1500    // touch within this = wake
#define HEAT_WAKE_DELTA_C          11.0f   // scene > ambient+this (~20F) = heat
#define HEAT_WAKE_SAMPLES          3       // consecutive rising samples to wake
#define SESSION_END_AMBIENT_C      8.0f    // within this of ambient = cool
#define SESSION_END_STABLE_MS      600000  // cool + stable this long -> end
#define IDLE_SENSOR_PERIOD_MS      10000   // IDLE: one frame per 10 s (§8)
#define SESSION_RING_SIZE          10      // last N session summaries (§10)
