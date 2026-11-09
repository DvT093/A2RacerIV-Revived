#!/usr/bin/env python3
"""Fill in the jump tables Ghidra left empty (symbols/jumptables.tsv).

For each `jmp dword ptr [4*reg + T]` without targets: the bound comes from
a `cmp reg, N` + `ja` just before (N+1 entries); without one, entries are
read while they point into .text close to the jump (MSVC memcpy tails);
zero entries are unused cases and skipped.
Prints rows for re/jumptables_manual.tsv.

Usage: python3 -I re/scripts/resolve_jumptables.py iso/Data/spel.dat symbols/jumptables.tsv
"""
import re
import struct
import subprocess
import sys

exe, tsv = sys.argv[1], sys.argv[2]
data = open(exe, "rb").read()
pe = struct.unpack_from("<I", data, 0x3C)[0]
nsec = struct.unpack_from("<H", data, pe + 6)[0]
optsz = struct.unpack_from("<H", data, pe + 20)[0]
base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
secs = []
for i in range(nsec):
    o = pe + 24 + optsz + i * 40
    name = data[o:o + 8].rstrip(b"\0").decode()
    vsz, va, rsz, raw = struct.unpack_from("<IIII", data, o + 8)
    secs.append((name, base + va, vsz, raw, rsz))
text = next(s for s in secs if s[0] == ".text")

def read32(va):
    for _, sva, vsz, raw, rsz in secs:
        if sva <= va < sva + min(vsz, rsz):
            return struct.unpack_from("<I", data, raw + va - sva)[0]
    return None

def disasm(start, stop):
    out = subprocess.run(["/opt/homebrew/opt/llvm@22/bin/llvm-objdump", "-d", "--no-show-raw-insn",
                          "--x86-asm-syntax=intel", "--start-address=%#x" % start,
                          "--stop-address=%#x" % stop, exe], capture_output=True, text=True).stdout
    rows = []
    for line in out.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\s+(.*)", line)
        if m:
            rows.append((int(m.group(1), 16), m.group(2).strip()))
    return rows

for line in open(tsv).read().splitlines()[1:]:
    jmp, _, targets = line.partition("\t")
    if targets.strip():
        continue
    j = int(jmp, 16)
    rows = disasm(j - 0x40, j + 8)
    ins = next((t for a, t in rows if a == j), "")
    m = re.search(r"jmp\s+dword ptr \[4\*(\w+) \+ 0x([0-9a-f]+)\]", ins)
    if not m:
        print("# %s: not a table jump: %s" % (jmp, ins))
        continue
    reg, table = m.group(1), int(m.group(2), 16)
    count, how = None, "unbounded"
    before = [t for a, t in rows if a < j][-6:]
    for k, t in enumerate(before):
        c = re.match(r"cmp\s+(\w+), (0x[0-9a-f]+|\d+)$", t)
        if c and c.group(1) == reg and any(x.startswith("ja") for x in before[k + 1:]):
            count, how = int(c.group(2), 0) + 1, "cmp %s" % c.group(2)
    tg = []
    for i in range(count or 256):
        v = read32(table + 4 * i)
        if v == 0 and count is None:
            continue  # unused case: a hole in the table
        if v is None or not (text[1] <= v < text[1] + text[2]):
            break
        if count is None and abs(v - j) > 0x2000:
            break
        if v not in tg:
            tg.append(v)
    if not tg:
        print("# %s: no targets at %08X" % (jmp, table))
        continue
    print("# %s: [%08X + %s*4], %s, %d entries" % (jmp, table, reg, how, len(tg)))
    print("%s\t%s" % (jmp, ",".join("%08X" % v for v in tg)))
