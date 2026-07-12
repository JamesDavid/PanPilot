# PanPilot enclosure — design concept

> Status: **v1 model generated** (2026-07-12) — `hardware/enclosure/panpilot_enclosure.scad`,
> print parts `shell` / `lid` / `base` (+ `assembly` preview; renders in
> `hardware/enclosure/renders/`). Board dimensions were measured from Elecrow's
> factory STEP model (`hardware/ref/`, regenerate the mesh with `convert_stp.py`),
> the MLX90640-D55 from Waveshare's dimension drawing — no calipers needed after
> all. Fit is **pending a test print** (HARDWARE_TEST "ENCLOSURE"): the corner
> screw thread (assumed M2 per the factory model), the D55 hole diameter
> (assumed 2.5), and the USB-C cutout alignment are the bench-verify items.

PanPilot is a hands-free device that lives **above the cooktop**, aims the
thermal sensor **down at the pans**, and angles the touchscreen **toward the
cook**. That dual-aim is the whole design driver.

## The core geometry problem

The MLX90640 and the screen want to point in *different* directions:

- **Sensor** looks **down** at the burner plane — its 55°×35° FOV is the coverage
  cone (BAB variant, base spec §2.2).
- **Screen** faces **up / out** toward the cook standing at the range.

So the natural shape is a **wedge**: an angled front face carrying the 3.5"
screen (tilted ~25–35° to sit on the cook's standing eyeline), and a
**downward-canted lip** below/ahead of it holding the MLX90640 breakout aimed at
the burners. Sensor and screen are on separate faces of the same wedge, so their
aims are independent.

```
                cook's eyeline
                     ↑
        ┌───────────╱          ← screen face, tilted ~30° toward cook
        │  CrowPanel │
        │   (S3 PCB) │
        └──┐        ╱
           │  MLX  ╱             ← sensor lip, canted down at the burners
           └──────┘  ⌄ 55°×35° FOV cone
        ─────────────────────────  cooktop plane (pans)
```

## Coverage math (ties to the multi-pan feature)

At mount height `h` above the cooktop the FOV footprint is ≈ `1.04·h` wide ×
`0.63·h` deep (from the 55°×35° full angles).

| Mount height `h` | Footprint (W × D) | Notes |
|---|---|---|
| 30 cm | ~31 × 19 cm | ~one pan; tight |
| **40 cm** | **~42 × 25 cm** | **two adjacent front burners** — matches per-zone timers / split-screen |
| 45 cm | ~47 × 28 cm | comfortable two-burner |
| 55 cm+ | ~57 × 35 cm | pans get small in-frame; confidence drops |

The mount has to **hold that height** and keep the sensor cone unobstructed — no
lip clipping the 55° cone, and no lens cover that will fog or grease over.

## Mounting variants (choose one — swappable base in the model)

1. **Under-hood clip / bracket** *(recommended)* — mounts to the front underside
   of a range hood. Cleanest, fixed height, zero counter space. Matches the
   existing *BladeKey-Overhead* pattern.
2. **Gooseneck / clamp arm** — clamps to a shelf edge or backsplash rail;
   adjustable aim. Most flexible, more parts.
3. **Shelf / microwave-top stand** — a weighted wedge that sits on the shelf
   above the range looking down-forward. No mounting hardware.

## Constraints baked into the model

- **Material: PETG or ASA**, not PLA — it sits over a heat + steam source and PLA
  sags at ~50–60 °C. Vent slots on the back keep the ESP32-S3 under the 85 °C
  die interlock (S11).
- **Steam / grease:** sensor recessed behind a short downward shroud so
  condensation doesn't pool on the lens; screen face slightly overhung.
- **Access:** USB-C passthrough (top/back), reset/boot reachable, touchscreen
  fully exposed, optional battery pocket if run off a cell (M7 power path).
- **Sensor mount:** the MLX90640 is a separate breakout on a short I²C tail
  (SDA 15 / SCL 16). The wedge holds the CrowPanel PCB in the main cavity and the
  breakout in the front lip.

## OpenSCAD parameterization (all keyed off measured numbers)

```scad
// --- CrowPanel Advance 3.5" PCB + screen ---
board_w; board_h; board_thickness;
screen_active_w; screen_active_h; screen_offset_x; screen_offset_y;   // window cutout
mount_holes = [[x,y], ...]; mount_hole_dia; standoff_h;
usbc_edge; usbc_center; usbc_w; usbc_h;                               // port passthrough

// --- MLX90640 breakout ---
mlx_board_w; mlx_board_h; mlx_hole_pitch; sensor_tilt_deg;

// --- wedge geometry ---
screen_tilt_deg; wedge_depth; wall_thickness;
vent_slots = [...];

// --- mount ---
mount_type = "hood" | "gooseneck" | "shelf";
mount_height_mm;   // documents the FOV coverage assumption
```

Everything reprints as the fit dials in, so early prototypes cost only filament.

## Measurements needed before generating the model

The kickoff forbids guessing dimensions, so calipers on the actual hardware:

1. **CrowPanel PCB** — outline W × H, thickness, and the **mounting-hole pattern**
   (hole positions + diameters).
2. **Screen** — active-area W × H and its offset from the PCB edges (window cutout).
3. **USB-C port** — which edge, plus center offset + width/height.
4. **MLX90640 breakout** — board W × H + its mount-hole positions.
5. **Chosen mount** — which of the three variants, and the **height** you can
   mount it above the burners (sets the FOV coverage from the table above).

Give me those five and the parametric `.scad` + an STL can be generated in one
pass.
