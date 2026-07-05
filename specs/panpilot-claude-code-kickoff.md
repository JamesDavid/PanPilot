# PanPilot — Claude Code Kickoff Prompt

Paste everything below the line into Claude Code, started in an empty directory containing the two spec files.

---

You are implementing **PanPilot**, a hands-free IR pan-temperature monitor and cooking guidance device, milestone by milestone, from two authoritative specs in this directory:

- `panpilot-firmware-spec.md` — base firmware spec, milestones **M0–M6** (CrowPanel 3.5" ESP32 + MLX90640 thermal array, PlatformIO/Arduino, LVGL, no laser)
- `panpilot-phase2-to-ultimate-spec.md` (v1.4) — roadmap spec, milestones **M7 onward**, including Appendix A (`foodlib_seed.h`) and Appendix A.1 (`preplib_seed.h`) which you must use **verbatim**

Read both specs completely before writing anything. Where the specs conflict with your instincts, the specs win. Where the specs say "verify against the Elecrow wiki," actually fetch and verify — never guess pins.

## Repo setup (do this first)

1. `git init`, then create a **public** GitHub repo named `panpilot` under my account using `gh repo create` (public is required for GitHub Pages web flasher on the free plan). Default branch `main`.
2. Move the two spec files into `specs/`. They are read-only references — never edit them.
3. Create `CLAUDE.md` at repo root distilling the Working Agreements below so every future session inherits them.
4. Standard PlatformIO `.gitignore` (`.pio/`, `*.bin` except release artifacts, editor junk).
5. Set up the project skeleton per base spec §3 with both board environments (`crowpanel35_basic`, `crowpanel35_advance`) plus the additional envs described under Screenshots and Testing below.

## Working Agreements (also goes in CLAUDE.md)

1. **One milestone per cycle, strictly in this order:** M0 → M1 → M2 → M3 → M4 → M5 → M6, then per roadmap spec §0.2: M7 → M8 → M9 → M10 → M11 → M13 → M12.5 → M12 → then fork per §0.2 (Track B: M19 → M20; Track A: M14 → M18 as hardware exists; merge: M21). Do not start a milestone until the previous one is committed, pushed, and — where it has hardware acceptance criteria — verified by me on the bench.
2. **The milestone cycle is:** implement → all native tests green (`pio test -e native`) → both firmware envs compile clean → screenshots captured → README updated with the new features → commit → push → **stop and report**, listing exactly what I need to verify on hardware before you proceed. Wait for my go-ahead at every hardware gate.
3. **Commits:** one milestone = one commit minimum (more is fine for logical units), message format `M<id>: <summary>`. Tag each completed milestone `milestone/m<id>`. Push after every milestone.
4. **Hardware honesty:** you have no access to my bench. Anything requiring a physical device (flash, touch, sensor readings, thermal tests) goes into `HARDWARE_TEST.md` as a per-milestone checklist with expected results, and the milestone is "pending hardware verification" until I confirm. Never claim on-device behavior you cannot observe.
5. **Pin verification is task zero of M0:** fetch the Elecrow wiki page for my exact CrowPanel variant, record the source URL and date in a comment block in `include/board_pins.h`. Ask me which variant is on my bench (basic ESP32-WROVER vs Advance ESP32-S3) and which MLX90640 FOV variant (BAA 110° vs BAB 55°) before finalizing.
6. **Documentation discipline:** the README (see below) and `HARDWARE_TEST.md` are the only standing docs you maintain. **If you want to create any additional explanatory document, ask me first before creating it.** Code comments and the specs carry everything else.
7. **No scope invention.** If a milestone seems to need something the specs don't define, ask; don't improvise features. Safety-tagged items (base spec food-safety fields, roadmap spec §3.3 interlocks, `safeInternalF`, smoke-point clamps) are never simplified away, even temporarily.
8. **Every tunable in `app_config.h`** with units and spec-section provenance, per base spec §13.

## Screenshots (every milestone with UI changes)

Build an **LVGL desktop simulator environment** (`env:sim`, SDL2, native) that compiles the real `ui/` and `core/` code against a mock sensor source that replays scripted `PanReading` sequences (a synthetic preheat ramp, a food-added drop, an overheat, etc.). Then:

- `scripts/screenshots.py` (or a make target) drives the simulator through every screen/state reachable at the current milestone and saves PNGs to `docs/screenshots/m<id>/` using LVGL's snapshot API.
- Screenshots are committed with the milestone and embedded in the README next to the feature they show.
- Also implement an on-device fallback for later hardware milestones: a serial command `screenshot` that dumps the framebuffer (raw RGB565 + a tiny host-side converter script in `scripts/`) so I can capture real-device shots for the README when the simulator can't represent something (thermal view with a live pan).

The simulator env is also your fast iteration loop for UI work — use it constantly, not just for screenshots.

## README as a user manual

`README.md` is written **user-manual style** — for a person who bought/built a PanPilot, not for developers. Grow it every milestone. Target structure (sections appear as their features land):

1. What PanPilot is (one paragraph + hero screenshot)
2. Hardware: parts list, wiring (MLX90640 to CrowPanel), enclosure notes
3. Flashing: link to the web flasher (below) + manual PlatformIO instructions
4. First boot & aiming: the thermal view, tap-to-lock ROI, what confidence means
5. Modes: Thermometer, Target Assist, Presets — with screenshots and "what the screen is telling you" callouts
6. The food timer & cook database: how auto-start works, flip cues, the Under/Perfect/Over feedback loop, the internal-temp safety notes and why they can't be dismissed
7. Attention levels: what each beep/flash pattern means (table)
8. Learn Pan Mode walkthrough
9. Web interface: dashboard, thermal view in browser, Recipe Creator (wizard, prep steps, fat advice)
10. Home Assistant integration: what entities appear, example automation
11. Autopilot & the SSR box: what it is, the arming ceremony, every interlock explained in plain language, **prominent supervised-use warnings**
12. Troubleshooting & FAQ (stainless pans read low, cleaning the sensor shield, aim lost, etc.)
13. Developer appendix (brief): repo layout, running tests, simulator, contributing

Keep it factual and instructional; screenshots inline; no marketing voice. A "safety" callout style is used consistently for anything involving heat, poultry/ground-meat temps, or the SSR box.

## Web flasher (GitHub Pages)

Set up during **M0** and keep it working from then on:

- GitHub Actions workflow: on every push to `main` and every `milestone/*` tag, build both firmware envs, produce merged single-address binaries with `esptool merge_bin` (bootloader + partitions + app at the correct offsets per chip), and publish to GitHub Pages.
- Pages site: minimal static `index.html` using **ESP Web Tools** (`<esp-web-install-button>`) with a `manifest.json` offering both variants — chipFamily `ESP32` for the basic panel and `ESP32-S3` for the Advance — labeled clearly so users pick their board. Show the firmware version (`git describe --tags`) on the page.
- Enable Pages via `gh` (deploy from the Actions artifact). Verify the workflow is green and the page loads before calling M0 done.
- README's Flashing section links here as the primary install path.

## Milestone gates that need me

Flag these explicitly when you reach them and wait:

- **M0:** which board variant + FOV variant I have; confirm repo created under the right account; I verify the Pages flasher loads and flashes.
- **M1:** I verify thermal view against a hot mug and a reference thermometer.
- **M3/M4:** I run the live-burner acceptance cooks from the specs.
- **M12.5:** I review any deviation you propose from Appendix A values (you may not change them unilaterally).
- **M14+ (SSR box):** entirely hardware-gated; you write firmware and `HARDWARE_TEST.md` procedures, I build and test the box. Interlock truth-table tests (roadmap §3.3) must be green in `pio test` before any control code runs on my hardware.

## Start now

Begin with M0: repo + CI + Pages flasher skeleton + board bring-up code + simulator env + initial README with sections 1–3 stubbed. Ask me the M0 questions (board variant, FOV variant, GitHub account) as your first message, and set up everything that doesn't depend on the answers while you wait.
