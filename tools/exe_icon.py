#!/usr/bin/env python3
"""Writes the biggest icon of a Windows EXE as a PNG (for the .app icon).
Usage: exe_icon.py EXE OUT.png"""
import struct, sys, zlib
d = open(sys.argv[1], 'rb').read()
pe = struct.unpack_from('<I', d, 0x3c)[0]
nsec = struct.unpack_from('<H', d, pe + 6)[0]; optsz = struct.unpack_from('<H', d, pe + 20)[0]
rva_res = struct.unpack_from('<I', d, pe + 24 + 112)[0] if struct.unpack_from('<H', d, pe + 24)[0] == 0x10b else None
secs = [struct.unpack_from('<IIII', d, pe + 24 + optsz + 40 * i + 8) for i in range(nsec)]
def off(rva):
    for vs, va, rs, ro in secs:
        if va <= rva < va + max(vs, rs): return ro + rva - va
base = off(rva_res)
def entries(o):
    n = sum(struct.unpack_from('<HH', d, base + o + 12))
    return [struct.unpack_from('<II', d, base + o + 16 + 8 * i) for i in range(n)]
def leaf(o):
    rva, size = struct.unpack_from('<II', d, base + o)
    return d[off(rva):off(rva) + size]
icons = {}
for typ, sub in entries(0):
    if typ != 3: continue  # RT_ICON
    for name, sub2 in entries(sub & 0x7fffffff):
        for lang, dat in entries(sub2 & 0x7fffffff):
            icons[name] = leaf(dat)
# pick the largest DIB icon
best = max(icons.values(), key=lambda b: struct.unpack_from('<i', b, 4)[0] * struct.unpack_from('<H', b, 14)[0])
hs, w, h2, planes, bpp = struct.unpack_from('<IiiHH', best, 0)
h = h2 // 2; ncol = struct.unpack_from('<I', best, 32)[0] or (1 << bpp if bpp <= 8 else 0)
pal = [best[hs + 4 * i:hs + 4 * i + 4] for i in range(ncol)]
px = hs + 4 * ncol
stride = ((w * bpp + 31) // 32) * 4; mstride = ((w + 31) // 32) * 4
mask = px + stride * h
rows = []
for y in range(h - 1, -1, -1):
    row = bytearray()
    for x in range(w):
        if bpp == 32: b, g, r, a = best[px + y * stride + 4 * x:px + y * stride + 4 * x + 4]
        elif bpp == 24: b, g, r = best[px + y * stride + 3 * x:px + y * stride + 3 * x + 3]; a = 255
        else:
            bit = x * bpp; byte = best[px + y * stride + bit // 8]
            idx = (byte >> (8 - bpp - bit % 8)) & ((1 << bpp) - 1)
            b, g, r, _ = pal[idx]; a = 255
        if bpp != 32 and (best[mask + y * mstride + x // 8] >> (7 - x % 8)) & 1: a = 0
        row += bytes([r, g, b, a])
    rows.append(row)
scale = max(1, 512 // w)
raw = b''.join(b'\0' + bytes(c for px4 in [r[i:i + 4] for i in range(0, len(r), 4)] for _ in range(scale) for c in px4)
               for r in rows for _ in range(scale))
W = w * scale; H = h * scale
def chunk(t, b): return struct.pack('>I', len(b)) + t + b + struct.pack('>I', zlib.crc32(t + b))
png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', W, H, 8, 6, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b'')
open(sys.argv[2], 'wb').write(png)
print(w, h, bpp, '->', W, H)
