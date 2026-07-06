#!/usr/bin/env python3
"""Render PanPilot UI screenshots from the headless LVGL simulator.

Builds env:sim, runs the program for each registered scene, and writes PNGs to
docs/screenshots/m<id>/. These are *synthesized placeholder* images of the
on-device UI (kickoff §Screenshots) — replace with real device photos later.

Usage:  python scripts/screenshots.py

Scenes grow per milestone. sim_main.cpp takes:  <out.ppm> [scene]
"""
import subprocess
import sys
from pathlib import Path

from PIL import Image

# (scene-id passed to sim_main, output PNG relative to repo root)
# Stable flat paths — these screens evolve each milestone, so they aren't
# filed under a milestone dir (milestone-specific *new* screens can be).
SCENES = [
    ("home", "docs/screenshots/home.png"),        # home / Target Assist
    ("thermal", "docs/screenshots/thermal.png"),  # live thermal view
    ("ready", "docs/screenshots/ready.png"),      # full-screen READY alert
    ("presets", "docs/screenshots/presets.png"),  # preset picker
    ("learn", "docs/screenshots/learn.png"),       # Learn Pan Mode
    ("lastcook", "docs/screenshots/lastcook.png"), # Last Cook trace
    ("foods", "docs/screenshots/foods.png"),       # food picker
    ("cooking", "docs/screenshots/cooking.png"),   # food timer countdown
    ("multipan", "docs/screenshots/multipan.png"), # two-pan home
    ("settings", "docs/screenshots/settings.png"), # device settings
    ("feedback", "docs/screenshots/feedback.png"), # post-cook Under/Perfect/Over
]

ROOT = Path(__file__).resolve().parents[1]
PROGRAM = ROOT / ".pio" / "build" / "sim" / "program.exe"
if not PROGRAM.exists():
    PROGRAM = ROOT / ".pio" / "build" / "sim" / "program"   # non-Windows


def build() -> None:
    print("building env:sim ...")
    subprocess.run([sys.executable, "-m", "platformio", "run", "-e", "sim"],
                   cwd=ROOT, check=True)


def render() -> None:
    for scene, out in SCENES:
        out_path = ROOT / out
        out_path.parent.mkdir(parents=True, exist_ok=True)
        ppm = out_path.with_suffix(".ppm")
        subprocess.run([str(PROGRAM), str(ppm), scene], cwd=ROOT, check=True)
        Image.open(ppm).save(out_path)
        ppm.unlink(missing_ok=True)
        print(f"  {scene} -> {out}")


if __name__ == "__main__":
    build()
    render()
    print("done.")
