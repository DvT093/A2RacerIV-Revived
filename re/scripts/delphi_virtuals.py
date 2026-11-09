"""Virtual methods a Delphi class overrides or adds (VMT slots that differ
from its parent). Usage: delphi_virtuals.py EXE Class... -> 'ADDR Class_vSLOT'"""
import struct, sys, importlib.util, os
spec = importlib.util.spec_from_file_location('df', os.path.join(os.path.dirname(__file__), 'delphi_fields.py'))
# reuse helpers without running its CLI
src = open(spec.origin).read().split('for cls in sys.argv[2:]:')[0]
argv = sys.argv; sys.argv = argv[:2]; exec(src); sys.argv = argv
def nslots(v):
    # slots run until the next class's data; use the parent's count + extras up to 64
    return 64
for cls in sys.argv[2:]:
    v = vmts.get(cls)
    if not v: continue
    p = parent(v)
    for i in range(0, 0x140, 4):
        try:
            a = u32(v + i)
            b = u32(p + i) if p else None
        except Exception: break
        if a != b and a and off(a) is not None and 0x401000 <= a < 0x4b0000:
            print(f'{a:08X} {cls}_v{i:03X}')
