"""0.24.2 aesthetics: the reward chest as a chest. Writes SourceArt/Meshes/SM_Chest.obj (a plank box under a domed lid) and SM_ChestTrim.obj (its gold bands, rims, corner
braces and lock plates, drawn as a second, metal piece) and their extents in SourceArt/mesh_bounds.json. Plain Python, no packages:
    python Tools/make_chest_meshes.py
The old SM_Chest (Tools/generate_source_art.py) was a box with its lid cylinder left standing upright through it, so the chest drew as a stepped octagonal block. Same footprint
(115 x 170 cm) and the same placement in the game (centre 52 cm up, scale 1.2): the base now sits on the floor. Centimetres, Z up; every face is a triangle with its UV and normal,
wound counter-clockwise seen from outside (the convention of the other source meshes). The planks of the wood run along the chest; UVs are box-mapped at 66 cm per sheet.
"""
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "SourceArt"
HALF_X, HALF_Y = 57.5, 85.0          # the footprint of the old chest
BOTTOM = -52.0 / 1.2                 # placed 52 cm up at scale 1.2 (ASWorld), so the base meets the floor
LID_BASE = BOTTOM + 60.0             # a 60 cm plank body
LID_RX, LID_RZ = HALF_X + 1.5, 32.0  # the domed lid overhangs the body a little
SHEET = 66.0                         # cm of wood per texture sheet (four planks)
ARC = 16                             # segments round the lid


class Mesh:
    def __init__(self):
        self.v, self.vt, self.vn, self.f = [], [], [], []

    def tri(self, a, b, c, na, nb, nc, ua, ub, uc):
        """One triangle, wound so its geometric normal agrees with the given (outward) normals."""
        e1 = [b[i] - a[i] for i in range(3)]
        e2 = [c[i] - a[i] for i in range(3)]
        g = [e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]]
        area = math.sqrt(sum(x * x for x in g))
        assert area > 1e-6, "degenerate triangle"
        n = [na[i] + nb[i] + nc[i] for i in range(3)]
        if sum(g[i] * n[i] for i in range(3)) < 0:
            b, c, nb, nc, ub, uc = c, b, nc, nb, uc, ub
        corners = []
        for p, nn, uv in ((a, na, ua), (b, nb, ub), (c, nc, uc)):
            self.v.append(p)
            self.vn.append(nn)
            self.vt.append(uv)
            corners.append(len(self.v))
        self.f.append(corners)

    def quad(self, a, b, c, d, n, uv):
        """A flat quad a-b-c-d (in order round its edge) with one normal; uv maps a point to its texture coordinate."""
        self.tri(a, b, c, n, n, n, uv(a), uv(b), uv(c))
        self.tri(a, c, d, n, n, n, uv(a), uv(c), uv(d))

    def box(self, lo, hi, faces=("+x", "-x", "+y", "-y", "+z", "-z")):
        x0, y0, z0 = lo
        x1, y1, z1 = hi
        p = lambda x, y, z: (x, y, z)
        if "+x" in faces: self.quad(p(x1, y0, z0), p(x1, y1, z0), p(x1, y1, z1), p(x1, y0, z1), (1, 0, 0), side_x)
        if "-x" in faces: self.quad(p(x0, y0, z0), p(x0, y0, z1), p(x0, y1, z1), p(x0, y1, z0), (-1, 0, 0), side_x)
        if "+y" in faces: self.quad(p(x0, y1, z0), p(x0, y1, z1), p(x1, y1, z1), p(x1, y1, z0), (0, 1, 0), side_y)
        if "-y" in faces: self.quad(p(x0, y0, z0), p(x1, y0, z0), p(x1, y0, z1), p(x0, y0, z1), (0, -1, 0), side_y)
        if "+z" in faces: self.quad(p(x0, y0, z1), p(x1, y0, z1), p(x1, y1, z1), p(x0, y1, z1), (0, 0, 1), top_z)
        if "-z" in faces: self.quad(p(x0, y0, z0), p(x0, y1, z0), p(x1, y1, z0), p(x1, y0, z0), (0, 0, -1), top_z)

    def write(self, name):
        lines = ["# %s: written by Tools/make_chest_meshes.py" % name, "o " + name]
        lines += ["v %.6f %.6f %.6f" % tuple(p) for p in self.v]
        lines += ["vt %.6f %.6f" % tuple(t) for t in self.vt]
        lines += ["vn %.6f %.6f %.6f" % tuple(n) for n in self.vn]
        lines += ["f %d/%d/%d %d/%d/%d %d/%d/%d" % (a, a, a, b, b, b, c, c, c) for a, b, c in self.f]
        (ROOT / "Meshes" / (name + ".obj")).write_text("\n".join(lines) + "\n", newline="\n")
        return [max(p[i] for p in self.v) - min(p[i] for p in self.v) for i in range(3)], [min(p[i] for p in self.v) for i in range(3)], [max(p[i] for p in self.v) for i in range(3)]


# box-mapped wood: on the long faces and the lid the planks (they run along the texture's v) run along the chest (y); on the ends they stand upright
side_x = lambda p: (p[2] / SHEET, p[1] / SHEET)
side_y = lambda p: (p[0] / SHEET, p[2] / SHEET)
top_z = lambda p: (p[0] / SHEET, p[1] / SHEET)


def lid_point(t, rx=LID_RX, rz=LID_RZ):
    """A point of the lid's arch at angle t (0 = the +x side, pi = the -x side) and its outward normal."""
    x, z = rx * math.cos(t), LID_BASE + rz * math.sin(t)
    nx, nz = math.cos(t) / rx, math.sin(t) / rz
    k = math.hypot(nx, nz)
    return x, z, (nx / k, 0.0, nz / k)


def arch(mesh, y0, y1, rx, rz, caps=True, bottom=True):
    """The domed lid (or a band over it) from y0 to y1: its curved top, both end caps and the flat underside."""
    pts = [lid_point(math.pi * i / ARC, rx, rz) for i in range(ARC + 1)]
    run = [0.0]
    for i in range(ARC):
        run.append(run[-1] + math.hypot(pts[i + 1][0] - pts[i][0], pts[i + 1][1] - pts[i][1]))
    for i in range(ARC):
        (xa, za, na), (xb, zb, nb) = pts[i], pts[i + 1]
        ua, ub = run[i] / SHEET, run[i + 1] / SHEET
        a, b, c, d = (xa, y0, za), (xb, y0, zb), (xb, y1, zb), (xa, y1, za)
        mesh.tri(a, b, c, na, nb, nb, (ua, y0 / SHEET), (ub, y0 / SHEET), (ub, y1 / SHEET))
        mesh.tri(a, c, d, na, nb, na, (ua, y0 / SHEET), (ub, y1 / SHEET), (ua, y1 / SHEET))
    if caps:
        for y, ny in ((y0, -1.0), (y1, 1.0)):
            centre = (0.0, y, LID_BASE)
            for i in range(ARC):
                (xa, za, _), (xb, zb, _) = pts[i], pts[i + 1]
                n = (0.0, ny, 0.0)
                mesh.tri(centre, (xa, y, za), (xb, y, zb), n, n, n, side_y(centre), side_y((xa, y, za)), side_y((xb, y, zb)))
    if bottom:
        n = (0.0, 0.0, -1.0)
        mesh.quad((-rx, y0, LID_BASE), (-rx, y1, LID_BASE), (rx, y1, LID_BASE), (rx, y0, LID_BASE), n, top_z)


def chest():
    m = Mesh()
    m.box((-HALF_X, -HALF_Y, BOTTOM), (HALF_X, HALF_Y, LID_BASE), ("+x", "-x", "+y", "-y", "-z"))  # its top is under the lid
    arch(m, -HALF_Y - 1.5, HALF_Y + 1.5, LID_RX, LID_RZ)
    return m


def span(a, b):
    return min(a, b), max(a, b)


def trim():
    m = Mesh()
    t = 2.2  # how far the metal stands proud of the wood
    # two bands up the long faces and over the lid, near each end (the face against the wood is left out)
    for yc in (-HALF_Y + 24.0, HALF_Y - 24.0):
        y0, y1 = yc - 5.5, yc + 5.5
        m.box((HALF_X, y0, BOTTOM), (HALF_X + t, y1, LID_BASE), ("+x", "+y", "-y", "+z", "-z"))
        m.box((-HALF_X - t, y0, BOTTOM), (-HALF_X, y1, LID_BASE), ("-x", "+y", "-y", "+z", "-z"))
        arch(m, y0, y1, LID_RX + t, LID_RZ + t, caps=True, bottom=False)
    # a rim round the top of the body (under the lid) and round its foot; the long sides run past the corners, so their ends close the corners
    for z0, z1 in ((LID_BASE - 6.0, LID_BASE), (BOTTOM, BOTTOM + 7.0)):
        m.box((HALF_X, -HALF_Y - t, z0), (HALF_X + t, HALF_Y + t, z1), ("+x", "+y", "-y", "+z", "-z"))
        m.box((-HALF_X - t, -HALF_Y - t, z0), (-HALF_X, HALF_Y + t, z1), ("-x", "+y", "-y", "+z", "-z"))
        m.box((-HALF_X, HALF_Y, z0), (HALF_X, HALF_Y + t, z1), ("+y", "+z", "-z"))
        m.box((-HALF_X, -HALF_Y - t, z0), (HALF_X, -HALF_Y, z1), ("-y", "+z", "-z"))
    # L-shaped braces down the four upright corners, between the rims
    z0, z1 = BOTTOM + 7.0, LID_BASE - 6.0
    for sx in (-1, 1):
        for sy in (-1, 1):
            fx, fy = ("+x" if sx > 0 else "-x"), ("+y" if sy > 0 else "-y")
            back_y = "-y" if sy > 0 else "+y"
            back_x = "-x" if sx > 0 else "+x"
            xa, xb = span(sx * HALF_X, sx * (HALF_X + t))
            ya, yb = span(sy * (HALF_Y - 9.0), sy * (HALF_Y + t))
            m.box((xa, ya, z0), (xb, yb, z1), (fx, fy, back_y))     # on the long face, round the corner
            xa, xb = span(sx * (HALF_X - 9.0), sx * HALF_X)
            ya, yb = span(sy * HALF_Y, sy * (HALF_Y + t))
            m.box((xa, ya, z0), (xb, yb, z1), (fy, back_x))         # on the end face
    # a lock plate on each long face, over the seam of the lid, and its hasp
    for sx in (-1, 1):
        x0, x1 = sorted((sx * HALF_X, sx * (HALF_X + 4.0)))
        m.box((x0, -12.0, LID_BASE - 20.0), (x1, 12.0, LID_BASE + 6.0))
        h0, h1 = sorted((sx * (HALF_X + 4.0), sx * (HALF_X + 7.0)))
        m.box((h0, -4.0, LID_BASE - 12.0), (h1, 4.0, LID_BASE - 2.0))
    return m


def main():
    bounds_path = ROOT / "mesh_bounds.json"
    bounds = json.loads(bounds_path.read_text())
    for name, mesh in (("SM_Chest", chest()), ("SM_ChestTrim", trim())):
        extent, lo, hi = mesh.write(name)
        bounds[name] = {"min": [round(x, 6) for x in lo], "max": [round(x, 6) for x in hi], "extent": [round(x, 6) for x in extent]}
        print("%s: %d triangles, extent %s" % (name, len(mesh.f), [round(x, 2) for x in extent]))
    bounds_path.write_text(json.dumps(bounds, indent=2) + "\n", newline="\n")


if __name__ == "__main__":
    main()
