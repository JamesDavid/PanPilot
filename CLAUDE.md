# CLAUDE.md — PanPilot working agreements

PanPilot is a hands-free IR pan-temperature monitor + cooking-guidance device
built milestone by milestone from two authoritative specs in `specs/`:

- `specs/panpilot-firmware-spec.md` — base firmware, **M0–M6** (CrowPanel 3.5" +
  MLX90640, PlatformIO/Arduino, LVGL, no laser).
- `specs/panpilot-phase2-to-ultimate-spec.md` (v1.4) — roadmap, **M7 onward**,
  incl. Appendix A (`foodlib_seed.h`) and Appendix A.1 (`preplib_seed.h`), which
  must be used **verbatim**.

**The specs are read-only and authoritative. Where they conflict with instinct,
the specs win. Where a spec says "verify against the Elecrow wiki," actually
fetch and verify — never guess pins.** `specs/` also holds the kickoff prompt
and reference dumps.

## Milestone order (strict)

M0 → M1 → M2 → M3 → M4 → M5 → M6, then per roadmap §0.2:
M7 → M8 → M9 → M10 → M11 → M13 → M12.5 → M12 → fork
(Track B: M19 → M20; Track A: M14 → M18 as hardware exists; merge: M21).

Do not start a milestone until the previous one is committed, pushed, and — where
it has hardware acceptance criteria — verified by James on the bench.

## The milestone cycle

implement → native tests green (`pio test -e native`, in CI) → both firmware
envs compile clean → screenshots captured → README updated → commit → push →
**stop and report**, listing exactly what James must verify on hardware. Wait for
go-ahead at every hardware gate.

- **Commits:** ≥1 per milestone, message `M<id>: <summary>`. Tag each completed
  milestone `milestone/m<id>`. Push after every milestone.
- **Hardware honesty:** no bench access here. Anything needing a physical device
  goes in `HARDWARE_TEST.md` as a per-milestone checklist with expected results;
  the milestone stays "pending hardware verification" until James confirms. Never
  claim on-device behavior that wasn't observed.
- **Docs discipline:** `README.md` (user-manual style) and `HARDWARE_TEST.md` are
  the only standing docs. Ask before creating any other explanatory doc.
- **No scope invention.** If a milestone seems to need something the specs don't
  define, ask. Safety-tagged items (food-safety fields, §3.3 interlocks,
  `safeInternalF`, smoke-point clamps) are never simplified away, even briefly.
- **Every tunable in `include/app_config.h`** with units + spec-section
  provenance (base spec §13). No magic numbers in code.
- **All GPIOs in `include/board_pins.h`** only (base spec §2.1).

## Architecture rules (base spec §3, §4)

- `core/` and `sensor/frame_analysis` contain **no Arduino/LVGL includes** —
  pure C++ on plain structs, unit-tested via `pio test -e native`.
- One-directional data flow: SensorTask → LogicTask → UITask; UI intents return
  via a command queue. No shared mutable state outside those channels.
- All internal math in °C; convert to °F only at the display layer.
- No blocking delays in tasks; no per-frame dynamic allocation after init.

## Decisions resolved (session 2026-07-05)

- **Bench board: CrowPanel Advance 3.5" (ESP32-S3-WROOM-1-N16R8)** — NOT the
  basic WROVER. Pins verified from Elecrow wiki + factory repo; see the
  provenance block in `board_pins.h`. Flasher chipFamily = **ESP32-S3**.
- **Sensor FOV: MLX90640 BAB 55°×35°** (spec §2.2 preferred).
- **Display stack: LovyanGFX + GT911**, not TFT_eSPI+XPT2046. The spec named
  TFT_eSPI assuming the basic board; the Advance S3 panel is LovyanGFX in
  Elecrow's own demo and in the user's BladeKey-Overhead. Multi-board HAL follows
  the user's `Board.h` capability-flag idiom.
- **Repo:** github.com/JamesDavid/panpilot (public, `main`). Web flasher on
  GitHub Pages via ESP Web Tools.
- **No host C++ compiler on the bench machine** (no gcc/clang/MSVC/SDL2). So:
  firmware compiles locally (PlatformIO Xtensa toolchain), but **`pio test -e
  native` and the SDL simulator run in CI (GitHub Actions/Ubuntu), which is the
  source of truth** for native tests + screenshots. A local MinGW can be added
  later for fast local iteration.
- **Additional (secondary) targets:** CrowPanel Advance 5" v1.1 (`../cyd-radio`)
  and v1.2 (`../BladeKey-Overhead`) — RGB-parallel ST7262. Pin maps captured in
  `board_pins.h`; RGB display HAL is a follow-up after the 3.5" bench board works.

## Milestone gates that need James

- **M0:** verify the Pages flasher loads + flashes the S3 board.
- **M1:** verify thermal view vs a hot mug + reference thermometer.
- **M3/M4:** run the live-burner acceptance cooks.
- **M12.5:** review any proposed deviation from Appendix A values (never change
  them unilaterally).
- **M14+ (SSR box):** hardware-gated; interlock truth-table tests (§3.3) must be
  green in `pio test` before any control code runs on hardware.
