"""Write Models/Shapes/ground_segment.p3d: one flat MLOD quad for the on-ground zone ring.

2 m along +Z (the ring's tangent), 1 m along X, at y = 0, visible from above, all of it in a
"camo" named selection so script can recolour it through hiddenSelections. No geometry LOD, so
it has no collision and raycasts pass through it.

Conventions read from Models/Shapes/zone.p3d: the visible side is the one cross(p1-p0, p2-p0)
points AWAY from, and the stored normal points away from it too.
"""
import os
import struct

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "Models", "Shapes", "ground_segment.p3d")

HALF_WIDTH = 0.5
HALF_LENGTH = 1.0
TEXTURE = "#(argb,8,8,3)color(1,0,0,0.35,ca)"

#--- Counter-clockwise seen from above (+Y), so cross(p1-p0, p2-p0) is -Y: visible from above.
POINTS = [
    (-HALF_WIDTH, 0.0, -HALF_LENGTH),
    (HALF_WIDTH, 0.0, -HALF_LENGTH),
    (HALF_WIDTH, 0.0, HALF_LENGTH),
    (-HALF_WIDTH, 0.0, HALF_LENGTH),
]
UVS = [(0.0, 1.0), (1.0, 1.0), (1.0, 0.0), (0.0, 0.0)]
NORMAL = (0.0, -1.0, 0.0)


def asciiz(text):
    return text.encode("latin-1") + b"\0"


def tag(name, payload):
    return b"\x01" + asciiz(name) + struct.pack("<I", len(payload)) + payload


def build():
    out = bytearray()
    out += b"MLOD" + struct.pack("<II", 257, 1)
    out += b"P3DM" + struct.pack("<IIIIII", 28, 256, len(POINTS), 1, 1, 0)
    for x, y, z in POINTS:
        out += struct.pack("<fffI", x, y, z, 0)
    out += struct.pack("<fff", *NORMAL)

    out += struct.pack("<I", 4)
    for i in range(4):
        u, v = UVS[i]
        out += struct.pack("<IIff", i, 0, u, v)
    out += struct.pack("<I", 0)
    out += asciiz(TEXTURE) + asciiz("")

    out += b"TAGG"
    out += tag("camo", bytes([1] * len(POINTS)) + bytes([1]))
    uvset = struct.pack("<I", 0) + b"".join(struct.pack("<ff", u, v) for u, v in UVS)
    out += tag("#UVSet#", uvset)
    out += tag("#EndOfFile#", b"")
    out += struct.pack("<f", 1.0)
    return bytes(out)


if __name__ == "__main__":
    data = build()
    with open(OUT, "wb") as f:
        f.write(data)
    print("wrote %s (%d bytes)" % (os.path.normpath(OUT), len(data)))
