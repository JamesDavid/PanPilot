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
#define MOVED_JUMP_PX              4       // centroid jump -> PAN MOVED (§6.4)
#define FOOD_DROP_C                15.0f  // panTemp drop -> FOOD ADDED (§6.4)
