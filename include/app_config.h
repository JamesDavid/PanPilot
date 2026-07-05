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
