#!/usr/bin/env python3
"""Stand-in for the race binary (tests of the menu): reads the game.ini the
menu wrote and writes a result. FAKE_RESULT=win|qualify|lose|caught|escape|abort."""
import os, sys, re
user = os.path.join(os.environ.get('XDG_DATA_HOME', ''), 'a2r4', 'install', 'game.ini')
txt = open(user, encoding='latin1').read()
mode = os.environ.get('FAKE_RESULT', 'win')
def setkey(txt, sec, key, val):
    m = re.search(r'(?im)^\[' + re.escape(sec) + r'\]\s*$', txt)
    if not m: return txt + f'\n[{sec}]\n{key}={val}\n'
    end = re.search(r'(?m)^\[', txt[m.end():]); stop = m.end() + (end.start() if end else len(txt) - m.end())
    body = txt[m.end():stop]
    if re.search(r'(?im)^' + key + r'=', body): body = re.sub(r'(?im)^' + key + r'=.*$', f'{key}={val}', body)
    else: body = f'\n{key}={val}' + body
    return txt[:m.end()] + body + txt[stop:]
fin = 1 if mode in ('win', 'lose', 'qualify', 'escape') else 0
me = {'win': 100000, 'qualify': 175000}.get(mode, 200000)
for k, v in [('FINISHED', fin), ('TRACKTIME', me), ('MONEY', 1234), ('TURBO_AMOUNT', 60),
             ('LAP_1_TIME', 40000), ('Out_Of_Time', 1 if mode == 'caught' else 0), ('Total_Loss', 0)]:
    txt = setkey(txt, 'PLAYER', k, v)
for i, (car, t) in enumerate([(0, 150000), (1, 160000), (3, 170000), (4, 180000)]):
    txt = setkey(txt, 'RACERS', str(i), f'{car},0,{t}')
txt = setkey(txt, 'RACERS', 'count', 4)
open(user, 'w', encoding='latin1').write(txt)
