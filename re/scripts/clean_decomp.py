"""Strips Delphi/Ghidra noise from a decompile (stack bookkeeping, debug log
calls) so the logic is readable. Usage: clean_decomp.py IN.c > OUT.c"""
import re, sys
skip = [r'^(puStack|uStack|iStack|pcStack|piStack|ppcStack|puVar\d+|local_\w+)\w* = (\(.*?\))?\s*(0x[0-9a-f]+|&UNK_[0-9a-f]+|&LAB_[0-9a-f]+|&stack0x[0-9a-f]+|\*in_FS_OFFSET|&\w+Stack_\w+|\w+Stack_\w+);$',
        r'^(undefined|int|uint|char|byte|bool|code|HWND|LPCSTR|short|ushort|float|double)\S*\s+\**\w+(\s*\[\d+\])?;$',
        r'^\*in_FS_OFFSET', r'^/\* WARNING', r'^iVar\d+ = \d+;$', r'^do \{$', r'^iVar\d+ = iVar\d+ \+ -1;$',
        r'^\} while \(iVar\d+ != 0\);$', r'FUN_00402ee4\(', r'FUN_00403e80\(', r'= FUN_0045f46c\(\);$',
        r'FUN_0045fb90\(', r'FUN_0045fc70\(', r'thunk_FUN_00402808', r'SBORROW4|SCARRY4', r'^$']
skip = [re.compile(s) for s in skip]
for l in open(sys.argv[1]):
    s = l.strip()
    if any(p.search(s) for p in skip): continue
    sys.stdout.write(l)
