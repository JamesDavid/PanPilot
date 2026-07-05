#!/usr/bin/env python3
"""Generate ESP Web Tools manifests + inject the firmware version into the page.

Usage:
    python scripts/gen_manifest.py <version> <web_dir>

Writes <web_dir>/manifest-advance.json (ESP32-S3) and manifest-basic.json
(ESP32), and replaces the __FW_VERSION__ token in <web_dir>/index.html.
Each manifest points at a single merged image at offset 0 (see merge_firmware.py).
"""
import json
import sys
from pathlib import Path

VARIANTS = {
    "advance": {
        "name": "PanPilot — CrowPanel Advance 3.5\" (ESP32-S3)",
        "chipFamily": "ESP32-S3",
        "bin": "firmware/crowpanel35_advance.bin",
    },
    "basic": {
        "name": "PanPilot — CrowPanel 3.5\" basic (ESP32)",
        "chipFamily": "ESP32",
        "bin": "firmware/crowpanel35_basic.bin",
    },
}


def main() -> None:
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    version, web = sys.argv[1], Path(sys.argv[2])

    for key, v in VARIANTS.items():
        manifest = {
            "name": v["name"],
            "version": version,
            "new_install_prompt_erase": True,
            "builds": [{
                "chipFamily": v["chipFamily"],
                "parts": [{"path": v["bin"], "offset": 0}],
            }],
        }
        (web / f"manifest-{key}.json").write_text(
            json.dumps(manifest, indent=2), encoding="utf-8"
        )
        print(f"wrote manifest-{key}.json ({version})")

    index = web / "index.html"
    html = index.read_text(encoding="utf-8").replace("__FW_VERSION__", version)
    index.write_text(html, encoding="utf-8")
    print(f"stamped index.html with {version}")


if __name__ == "__main__":
    main()
