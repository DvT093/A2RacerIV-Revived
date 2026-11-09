"""Annotates object-field accesses in a decompile with Delphi field names.
Usage: annotate_fields.py FIELDS.txt IN.c > OUT.c  (function names 'TClass_Method')"""
import re, sys
f = {}
for l in open(sys.argv[1]):
    c, o, n = l.split(); f.setdefault(c, {})[int(o, 16)] = n
cur = {}
out = []
for l in open(sys.argv[2]):
    m = re.match(r'===== (T\w+?)_', l)
    if m: cur = f.get(m.group(1), {})
    def a(m):
        o = int(m.group(2), 16)
        return m.group(0) + (f' /*{cur[o]}*/' if o in cur else '')
    def b(m):
        o = int(m.group(2), 16) * 4
        return m.group(0) + (f' /*{cur[o]}*/' if o in cur else '')
    l = re.sub(r'(param_1|iStack_8|piStack_8) \+ (0x[0-9a-f]+)\)', a, l)
    l = re.sub(r'(param_1|piStack_8)\[(0x[0-9a-f]+)\]', b, l)
    out.append(l)
sys.stdout.write(''.join(out))
