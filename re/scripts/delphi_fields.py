"""Delphi (D2-D7) published field tables: object offset -> component name per class,
following parent classes. Usage: delphi_fields.py EXE Class [Class...]
Needs the VMT addresses from delphi_vmt.py (re/menu_vmt.txt) or finds them itself."""
import struct, sys
d = open(sys.argv[1], 'rb').read()
pe = struct.unpack_from('<I', d, 0x3c)[0]
nsec = struct.unpack_from('<H', d, pe + 6)[0]; optsz = struct.unpack_from('<H', d, pe + 20)[0]
base = struct.unpack_from('<I', d, pe + 52)[0]
secs = [struct.unpack_from('<IIII', d, pe + 24 + optsz + 40 * i + 8) for i in range(nsec)]
def off(v):
    v -= base
    for vs, va, rs, ro in secs:
        if va <= v < va + max(vs, rs) and v - va < rs: return ro + v - va
def u32(v): return struct.unpack_from('<I', d, off(v))[0]
def sstr(v): o = off(v); return d[o + 1:o + 1 + d[o]].decode('latin1')
# find all VMTs: vmtSelfPtr (-76) points to itself
vmts = {}
for vs, va, rs, ro in secs:
    for o in range(ro, ro + rs - 80, 4):
        cand = base + va + (o - ro) + 76
        if struct.unpack_from('<I', d, o)[0] == cand:
            try:
                name = sstr(u32(cand - 44))
                if name.isidentifier(): vmts[name] = cand
            except Exception: pass
def parent(vmt):
    p = u32(vmt - 36)
    return u32(p) if p else 0
def fields(vmt):
    ft = u32(vmt - 56)
    if not ft: return []
    o = off(ft); n = struct.unpack_from('<H', d, o)[0]; o += 6; out = []
    for _ in range(n):
        fo = struct.unpack_from('<I', d, o)[0]; l = d[o + 6]
        out.append((fo, d[o + 7:o + 7 + l].decode('latin1'))); o += 7 + l
    return out
for cls in sys.argv[2:]:
    v = vmts.get(cls); res = []
    while v:
        try: res += fields(v); v = parent(v)
        except Exception: break
    for fo, nm in sorted(res): print(f'{cls} 0x{fo:x} {nm}')
