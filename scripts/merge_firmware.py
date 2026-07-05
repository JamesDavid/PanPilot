#!/usr/bin/env python3
"""Merge PlatformIO build outputs into a single flashable image for ESP Web Tools.

Usage:
    python scripts/merge_firmware.py <env> <chip> <out.bin>

<chip> is "esp32" or "esp32s3" (sets the bootloader offset: classic ESP32 boots
at 0x1000, ESP32-S3 at 0x0). Locates bootloader/partitions/boot_app0/app bins
under .pio/build/<env>/ and calls `esptool merge_bin` so the result flashes at
offset 0 (manifest offset 0). Standard Arduino-ESP32 layout.
"""
import subprocess
import sys
from pathlib import Path

# bootloader, partitions, boot_app0, app offsets by chip
OFFSETS = {
    "esp32":   {"bootloader": 0x1000},
    "esp32s3": {"bootloader": 0x0000},
    "esp32c3": {"bootloader": 0x0000},
}
PART_OFF = 0x8000
BOOTAPP_OFF = 0xE000
APP_OFF = 0x10000


def find_boot_app0() -> Path:
    home = Path.home()
    candidates = list(
        (home / ".platformio" / "packages").glob(
            "framework-arduinoespressif32*/tools/partitions/boot_app0.bin"
        )
    )
    if not candidates:
        sys.exit("boot_app0.bin not found — is the Arduino framework installed?")
    return candidates[0]


def main() -> None:
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    env, chip, out = sys.argv[1], sys.argv[2], sys.argv[3]
    if chip not in OFFSETS:
        sys.exit(f"unknown chip {chip!r}; expected one of {list(OFFSETS)}")

    build = Path(".pio") / "build" / env
    boot = build / "bootloader.bin"
    part = build / "partitions.bin"
    app = build / "firmware.bin"
    for p in (boot, part, app):
        if not p.exists():
            sys.exit(f"missing build output: {p}")
    boot_app0 = find_boot_app0()

    Path(out).parent.mkdir(parents=True, exist_ok=True)
    cmd = [
        sys.executable, "-m", "esptool", "--chip", chip, "merge_bin",
        "-o", out, "--flash_mode", "dio", "--flash_size", "16MB",
        f"0x{OFFSETS[chip]['bootloader']:x}", str(boot),
        f"0x{PART_OFF:x}", str(part),
        f"0x{BOOTAPP_OFF:x}", str(boot_app0),
        f"0x{APP_OFF:x}", str(app),
    ]
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=True)
    print(f"wrote {out}")


if __name__ == "__main__":
    main()
