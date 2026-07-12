// panpilot_enclosure.scad — parametric wedge enclosure for PanPilot.
// CrowPanel Advance 3.5" HW V1.2 + Waveshare MLX90640-D55 breakout.
//
// DIMENSION PROVENANCE (kickoff rule: never guess):
//  * Board: measured from Elecrow's factory STEP model
//    (hardware/ref/advance-hmi3_5-20260126.stp, fetched 2026-07-12; regenerate
//    the mesh with hardware/ref/convert_stp.py). Assembly envelope
//    98.8 x 63.4 x 16.61 mm; glass window 82.94 x 54.66 at the offsets below;
//    4x M3 hex standoffs on a 90.8 x 52.7 grid, factory-fitted with rear-entry
//    M2 screws (the lid reuses those threads with longer M2s).
//  * Sensor: Waveshare wiki dimension drawing (28.00 x 16.00 board, mount
//    holes 2.00 from the right edge / 2.00 from the bottom, 12.00 apart).
//  * Design intent: hardware/enclosure_concept.md (wedge: screen tilted to the
//    cook, sensor pod aimed down at the burners; PETG/ASA, vents, FOV cone).
//
// COORDINATES: board frame. +Z is the viewing direction (out of the glass);
// the lid seats at z=0, the front frame face is at z=depth. The bottom wall
// (y = -wall-clr) carries the sensor pod; on the shelf base the whole shell
// leans back `screen_tilt` degrees.
//
// PRINT PARTS:  part = "shell" | "lid" | "base" | "assembly" (preview)
// BENCH-VERIFY before a full print (HARDWARE_TEST "ENCLOSURE"):
//  - corner-screw thread (assumed M2, from the factory ZM_M2 screws)
//  - D55 mount-hole diameter (assumed 2.5; the drawing gives position only)
//  - USB-C cutout alignment on the right wall

/* ---------------- parameters ---------------- */
part = "assembly";        // what to render
show_board = true;        // ghost the factory board mesh in the assembly view

wall        = 3.0;        // PETG walls (heat + steam over a cooktop)
clr         = 0.6;        // fit clearance per side around the board envelope
screen_tilt = 30;         // deg from vertical, screen face toward the cook
sensor_aim  = 25;         // pod hinge-down angle from the shell's front plane;
                          // on the 30-deg shelf the lens ends up ~5 deg
                          // forward of straight down (0 = flush forward)
mount_type  = "shelf";    // "shelf" wedge base | "hood" bracket plate

/* -------- CrowPanel Advance 3.5 (from the STP) ---- */
brd   = [98.8, 63.4, 16.61];             // assembly envelope W x H x D
win   = [6.9, 3.4, 91.9, 60.0];          // glass window cut x0,y0,x1,y1 (+1mm margin)
holes = [[4.0, 5.35], [4.0, 58.05],      // M3 standoff centers (rear-entry M2
         [94.8, 5.35], [94.8, 58.05]];   //  screws per the factory model)
usb   = [15.0, 39.0, 4.5, 11.0];         // right-wall cutout y0,y1 and z0,z1
                                         //  measured off the two USB-C bodies
pwr_x = 80.5;                            // bottom cable-slot center (XH power
                                         //  connector lands at x 72.9..87.9)
rear_gap = 6.0;                          // lid inner face to board rear plane

/* ------------- MLX90640-D55 (Waveshare drawing) ------------- */
mlx       = [28.0, 16.0, 1.6];           // PCB W x H x thickness
mlx_holes = [[26.0, 2.0], [26.0, 14.0]]; // centers from the board's bottom-left
mlx_hole_d = 2.5;                        // BENCH-VERIFY (drawing omits it)
mlx_lens  = [19.5, 8.0];                 // lens center (scaled off the drawing)
mlx_fov   = 60;                          // aperture cone > the 55-deg FOV

/* ---------------- derived ---------------- */
cavity = [brd[0] + 2*clr, brd[1] + 2*clr];              // 100.0 x 64.6
outer  = [cavity[0] + 2*wall, cavity[1] + 2*wall];      // 106.0 x 70.6
depth  = brd[2] + rear_gap + wall;                      // z: lid seat -> front face
pod_w  = mlx[0] + 2*wall + 1.4;                         // sensor pod outer size
pod_h  = mlx[1] + 2*wall + 1.4;
pod_t  = wall + mlx[2] + 8;                             // lens plate + PCB + plug room

$fn = 48;

/* ================= board reference (ghost) ================= */
module board_ref() {
  // STP -> board frame: mirror Z (STP glass faces -Z) and re-zero the envelope
  // min corner to (0,0); the board's rear plane sits at z = rear_gap.
  translate([1.0, 2.35, rear_gap + 9.01]) mirror([0, 0, 1])
      import("../ref/crowpanel_adv35_board.stl");
}

/* ================= shell ================= */
module shell_body() {
  difference() {
    translate([-wall - clr, -wall - clr, 0]) cube([outer[0], outer[1], depth]);
    // board cavity (open toward the lid at z<0)
    translate([-clr, -clr, -1]) cube([cavity[0], cavity[1], brd[2] + rear_gap + 1]);
    // glass window through the front frame
    translate([win[0], win[1], brd[2] + rear_gap - 0.5])
        cube([win[2] - win[0], win[3] - win[1], wall + 1.5]);
    // right-wall USB-C opening (both ports in one rounded slot)
    translate([brd[0] + clr - 1, usb[0], rear_gap + usb[2]])
        cube([wall + 2, usb[1] - usb[0], usb[3] - usb[2]]);
    // bottom-wall cable slot for the XH power tail (exits down/back)
    translate([pwr_x - 9, -wall - clr - 1, rear_gap - 1])
        cube([18, wall + 2, 8]);
    // bottom-wall slot feeding the sensor pod's PH2.0 tail into the cavity
    translate([outer[0]/2 - wall - clr - 5, -wall - clr - 1, rear_gap + 1])
        cube([10, wall + 2, 8]);
  }
}

/* Sensor pod: a small box hinged off the shell's front-bottom edge, rotated
   `sensor_aim` degrees down from the front plane. Built with the lens facing
   local +Z, so after the hinge the lens looks down-forward; with the shell on
   the 30-deg shelf base the net aim is a few degrees forward of vertical
   (coverage table in enclosure_concept.md). */
module pod_body() {
  difference() {
    cube([pod_w, pod_h, pod_t]);
    // PCB pocket entered from the back (lens plate is the +z face)
    translate([wall + 0.4, wall + 0.4, -1])
        cube([mlx[0] + 0.6, mlx[1] + 0.6, 1 + pod_t - wall]);
    // lens aperture + FOV cone through the lens plate
    translate([wall + 0.7 + mlx_lens[0], wall + 0.7 + mlx_lens[1], 0]) {
      translate([0, 0, pod_t - wall - 0.01]) cylinder(d = 13, h = wall + 0.1);
      translate([0, 0, pod_t - 0.01])
          cylinder(d1 = 13, d2 = 13 + 2 * wall * tan(mlx_fov / 2), h = wall);
    }
    // screw bosses' pilot holes (self-tap into plastic through the PCB holes)
    for (h = mlx_holes)
        translate([wall + 0.7 + h[0], wall + 0.7 + h[1], pod_t - wall - 4])
            cylinder(d = mlx_hole_d - 0.4, h = 5);
    // cable exit toward the hinge edge (PH2.0 tail runs into the shell)
    translate([pod_w/2 - 5, pod_h - wall - 1, wall])
        cube([10, wall + 2, pod_t - 2*wall]);
  }
}

module sensor_pod() {
  // Hinge line: the shell's front-bottom edge. The body extends down-and-back
  // UNDER the floor (local -y after the rotation) so nothing overlaps the
  // glass; the lens plate (local +z) ends up looking `sensor_aim` degrees
  // forward of straight down in the shell frame.
  hx = outer[0]/2 - wall - clr - pod_w/2;
  // hinge sunk 1.4mm into the front-bottom edge so the pod FUSES to the shell
  // (an exact-touch joint leaves a disjoint volume -> detached print)
  translate([hx, -wall - clr + 1.4, depth - 1.4])
    rotate([90 - sensor_aim, 0, 0]) {
      translate([0, -pod_h, 0]) pod_body();
      // gusset wedge fusing the pod's top face back to the shell floor
      translate([0, -pod_h, pod_t - 0.01])
          cube([pod_w, pod_h - 2, wall * sin(sensor_aim) + 1.2]);
    }
}

module shell() {
  shell_body();
  sensor_pod();
}

/* ================= lid ================= */
module lid() {
  difference() {
    union() {
      translate([-wall - clr, -wall - clr, -wall]) cube([outer[0], outer[1], wall]);
      // bosses reaching the four standoff rear faces (at z = rear_gap)
      for (h = holes) translate([h[0], h[1], -1]) cylinder(d = 7, h = rear_gap + 1);
      // registration lip inside the cavity mouth
      translate([-clr + 0.2, -clr + 0.2, -1])
          linear_extrude(3) difference() {
            square([cavity[0] - 0.4, cavity[1] - 0.4]);
            offset(delta = -1.6) square([cavity[0] - 0.4, cavity[1] - 0.4]);
          }
    }
    // M2 clearance through the lid + bosses (threads live in the standoffs)
    for (h = holes) translate([h[0], h[1], -wall - 0.5])
        cylinder(d = 2.4, h = wall + rear_gap + 1);
    for (h = holes) translate([h[0], h[1], -wall - 0.01]) cylinder(d = 4.4, h = 1.4);
    // vent slots behind the module area (S11 die-temp headroom)
    for (i = [0:5]) translate([18 + i*12, 12, -wall - 0.5]) cube([6, 38, wall + 1]);
  }
}

/* ================= base ================= */
// Shelf-wedge dimensions (file scope: OpenSCAD forbids modules inside if{}).
base_H  = outer[1] * cos(screen_tilt) + 10;   // slope long enough for the shell
base_D  = base_H * tan(screen_tilt);          // horizontal run of the slope
base_bw = outer[0] + 12;                      // base width
base_lip = 7;                                 // cradle-rail height above face

// helper frame: origin on the slope-face center, +z = outward normal,
// +y = down-the-slope
module slope_frame() {
  translate([base_bw/2, base_D/2, base_H/2])
      rotate([-(90 - screen_tilt), 0, 0]) children();
}

module base() {
  if (mount_type == "shelf") {
    // Solid wedge whose inclined face carries the shell's lid at screen_tilt
    // from vertical, with cradle rails on three sides (the shell drops in and
    // rests on the bottom stop; no fasteners). Forward foot resists tipping.
    H = base_H; D = base_D; bw = base_bw; lip = base_lip;
    rotate([90, 0, 90]) linear_extrude(bw)
        polygon([[0, 0], [D + 24, 0], [D + 24, 6], [D + 4, 6], [0, H]]);
    slope_frame() {
      // side rails (sunk 2mm into the face so they fuse with the wedge)
      translate([-outer[0]/2 - 4, -outer[1]/2 - 2, -2])
          cube([4, outer[1] + 4, lip + 2]);
      translate([outer[0]/2, -outer[1]/2 - 2, -2])
          cube([4, outer[1] + 4, lip + 2]);
      // bottom stop with a center gap for the power/sensor cables
      for (sx = [-1, 1])
          translate([sx == -1 ? -outer[0]/2 - 4 : 12, outer[1]/2, -2])
              cube([outer[0]/2 - 8, 4, lip + 2]);
    }
  } else {
    // hood bracket: flat plate, two M4 slots, shell lid screws to it
    difference() {
      cube([outer[0], 42, wall + 2]);
      for (x = [20, outer[0] - 20]) hull() for (y = [12, 30])
          translate([x, y, -0.5]) cylinder(d = 4.4, h = wall + 3);
    }
  }
}

/* ================= assembly preview ================= */
module assembly() {
  color("SteelBlue") shell();
  color("SlateGray") lid();
  if (show_board) %board_ref();
}

if (part == "shell") shell();
else if (part == "lid") lid();
else if (part == "base") base();
else assembly();
