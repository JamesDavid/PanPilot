"""Convert the CrowPanel Advance 3.5 STEP model to STL and report geometry.

Outputs:
  - overall bounding box (board + screen assembly)
  - per-volume bounding boxes for the large features (PCB, glass, connectors)
  - a decimated STL for OpenSCAD visual reference
"""
import sys
import gmsh

STP = r"C:\Users\James\Desktop\PanPilot\hardware\ref\advance-hmi3_5-20260126.stp"
STL = r"C:\Users\James\Desktop\PanPilot\hardware\ref\crowpanel_adv35_board.stl"

gmsh.initialize()
gmsh.option.setNumber("General.Terminal", 0)
gmsh.option.setNumber("Geometry.OCCImportLabels", 1)
gmsh.open(STP)

xmin, ymin, zmin, xmax, ymax, zmax = gmsh.model.getBoundingBox(-1, -1)
print(f"OVERALL mm: X {xmax-xmin:.2f}  Y {ymax-ymin:.2f}  Z {zmax-zmin:.2f}")
print(f"  x[{xmin:.2f},{xmax:.2f}] y[{ymin:.2f},{ymax:.2f}] z[{zmin:.2f},{zmax:.2f}]")

vols = gmsh.model.getEntities(3)
print(f"volumes: {len(vols)}")
feats = []
for dim, tag in vols:
    bb = gmsh.model.getBoundingBox(dim, tag)
    dx, dy, dz = bb[3]-bb[0], bb[4]-bb[1], bb[5]-bb[2]
    name = ""
    try:
        name = gmsh.model.getEntityName(dim, tag)
    except Exception:
        pass
    feats.append((dx*dy*dz, tag, name, bb, (dx, dy, dz)))
feats.sort(reverse=True)
print("TOP 25 volumes by bbox size (mm):")
for volu, tag, name, bb, d in feats[:25]:
    print(f"  tag={tag:4d} {d[0]:7.2f} x {d[1]:7.2f} x {d[2]:6.2f}"
          f"  @x[{bb[0]:7.2f},{bb[3]:7.2f}] y[{bb[1]:7.2f},{bb[4]:7.2f}]"
          f" z[{bb[2]:6.2f},{bb[5]:6.2f}]  {name[:40]}")

# Coarse surface mesh -> STL (visual reference only).
gmsh.option.setNumber("Mesh.MeshSizeMax", 3.0)
gmsh.option.setNumber("Mesh.MeshSizeMin", 0.8)
gmsh.option.setNumber("Mesh.Algorithm", 6)
try:
    gmsh.model.mesh.generate(2)
    gmsh.write(STL)
    print(f"STL written: {STL}")
except Exception as e:
    print(f"mesh failed: {e}", file=sys.stderr)
gmsh.finalize()
