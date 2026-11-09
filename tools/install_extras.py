#!/usr/bin/env python3
"""Installeert de officiële extra's van Davilex (december 2000) in de port.

Davilex bood bezitters van A2 Racer IV een extra auto (de truck van KENT,
op de plek van de DeLorean) en een bonustrack (etappe 9, Engels platteland
met het Uffington White Horse en Avebury) aan als download. Die installers
staan in het Wayback-archief:

  http://www.davilex.nl/service/a2racer4/download/a2racer4_update_car.exe
  http://www.davilex.nl/service/a2racer4/download/a2racer4_update_track.exe

Leg ze in dl/extras/ (zie CLAUDE.md). Dit script voert ze niet uit: het
leest de bestanden eruit (Wise-installer, losse deflate-blokken), herkent
ze aan hun sha1 en zet ze in de spelmap van de port, de SwARc-overlay
($XDG_DATA_HOME/a2r4/install, standaard userdata/a2r4/install), zoals de
originele installer ze in de spelmap zette. game.ini uit de update nemen
we niet over (testinstelling van de makers: etappe 8, 1 ronde, HUD deels
uit); alleen de regel die de truck nodig heeft, [Portraits] 7=PLAYER_7,
komt in game.ini (anders stopt de race met "Element name for car 7 not
found in ini GAME.INI, header Portraits").

De modellen en textures van de bonustrack en de truck staan al op de CD;
de update levert alleen de kaart (etappe9.*) en de ini's. Het menu ziet de
truck aan [DOWNLOADCAR] Count in L0_ANIM.INI (Menu.exe 0x48e9f8), de track
aan etappe9.rmp.

Gebruik: python3 -I tools/install_extras.py [--dest map]
"""

import argparse
import hashlib
import os
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# (installer, bestand, sha1). Uit de installers gehaald op 9 nov 2026.
FILES = [
    ("a2racer4_update_car.exe", "car_cfg.ini", "1c133b617832fe26c6e3974538a503ef15c5e326"),
    ("a2racer4_update_car.exe", "car_snd.ini", "239e383e352f2ede7d43ea6007be1244ba797dc2"),
    ("a2racer4_update_car.exe", "damage.ini", "3003bade3ea9271a444ceeb9fe66fbf01a1c9227"),
    ("a2racer4_update_car.exe", "gen_anim.ini", "29d5066b7c33b5080955f8534ea2cdd9c6cf731a"),
    ("a2racer4_update_car.exe", "l0_anim.ini", "bd490b17bed6a11a9fe614952fcbb37c187ae228"),
    ("a2racer4_update_car.exe", "l1_anim.ini", "bd490b17bed6a11a9fe614952fcbb37c187ae228"),
    ("a2racer4_update_car.exe", "l2_anim.ini", "bd490b17bed6a11a9fe614952fcbb37c187ae228"),
    ("a2racer4_update_car.exe", "overlay7.ini", "d8d7b6d45bd31956a52af5af29e7b86c957b23d3"),
    ("a2racer4_update_car.exe", "tru.d3d", "4075f38f13f6e3f483d1e4109c364cbb0e504776"),
    ("a2racer4_update_car.exe", "tru_A00.d3d", "63995c3b1753cd5877e623a13abd740bf18a47fa"),
    ("a2racer4_update_car.exe", "tru_A01.d3d", "8648f8eff52cf2f3aedf7eb7d184686a86941019"),
    ("a2racer4_update_car.exe", "tru_A02.d3d", "a21a8bafa39030f7538949f04cd1298e5e9c1c33"),
    ("a2racer4_update_car.exe", "tru_A03.d3d", "d70af6945c0b16fca662fdb4c9e9f1b86317f0d9"),
    ("a2racer4_update_car.exe", "tru_A04.d3d", "93f32a2da538a8ec81886d501c03767d0ec3b969"),
    ("a2racer4_update_car.exe", "tru_A05.d3d", "6135919f2448dcfaa8e81ab1c860eabdc5e238b4"),
    ("a2racer4_update_car.exe", "tru_A06.d3d", "8c82d9c7d8309f35aa33f756db138091235e0aeb"),
    ("a2racer4_update_car.exe", "tru_A07.d3d", "7205b0a3de452b3955c67a50c283ca001f7f576e"),
    ("a2racer4_update_car.exe", "tru_A08.d3d", "beb53dd44fea6bab7d4db7c924a797934dfa6a47"),
    ("a2racer4_update_car.exe", "tru_A09.d3d", "95918130a0375b13016cd285ff381c6920bfafe8"),
    ("a2racer4_update_car.exe", "tru_A10.d3d", "00eb9805910f6a6924a6adc0cfaaa230fc4489a1"),
    ("a2racer4_update_car.exe", "tru_A11.d3d", "17ad927c8bd504aebfc1ded1067fdab5f615239e"),
    ("a2racer4_update_car.exe", "tru_bot.d3d", "b3db8acb07c3e5bd13cf791245613831a2fa2735"),
    ("a2racer4_update_car.exe", "tru_drv.d3d", "65eee430f86b514a97636bd992b053a8657caa53"),
    ("a2racer4_update_car.exe", "tru_ram.d3d", "570dfb26a5bb5783716958c8b3f9711e23e57193"),
    ("a2racer4_update_car.exe", "tru_wie.d3d", "3f205d1e63cc8e1df34c19e2fbfb6dc89159a0b8"),
    ("a2racer4_update_car.exe", "vdyns.dat", "181587a778c925cf1a916760037943554a4c9109"),
    ("a2racer4_update_track.exe", "e09_anim.ini", "1f4afd208fb05b1eb90b7003d4f6e97a57fbd10f"),
    ("a2racer4_update_track.exe", "etappe9.cmp", "ebffbe186bdb29dd4b8808d70396d7a02a765096"),
    ("a2racer4_update_track.exe", "etappe9.dmp", "095547d192f6aa7555915cfe57db0c9acdb95668"),
    ("a2racer4_update_track.exe", "etappe9.ini", "a3076e3f2a94aad38741d14f71e451efa02cda27"),
    ("a2racer4_update_track.exe", "etappe9.omp", "006ed3f121972ce26258d39479fce1f0c29da400"),
    ("a2racer4_update_track.exe", "etappe9.pmp", "d3560440802fa7525aff015f90483a3e78f2ccc3"),
    ("a2racer4_update_track.exe", "etappe9.pth", "8bc6634fc6dcad087eb776db4a44c501ec8a5ac8"),
    ("a2racer4_update_track.exe", "etappe9.rmp", "07a5db48e1a1e4204e90369d66eefc263d20034e"),
]


def streams(data):
    """Alle deflate-blokken in de installer (Wise zet elk bestand los)."""
    i = 0
    while i < len(data) - 16:
        d = zlib.decompressobj(-15)
        try:
            d.decompress(data[i:i + 64], 0)  # snel afwijzen
        except zlib.error:
            i += 1
            continue
        d = zlib.decompressobj(-15)
        try:
            res = d.decompress(data[i:])
        except zlib.error:
            i += 1
            continue
        if d.eof and len(res) > 64:
            yield res
        # Ook na een treffer de volgende byte proberen: een blok kan één byte
        # eerder al "geldig" beginnen (tru_ram.d3d kreeg zo een 0x6e ervoor).
        i += 1


def find_nocase(d, name):
    if os.path.isdir(d):
        for f in os.listdir(d):
            if f.lower() == name.lower():
                return f
    return None


def main():
    ap = argparse.ArgumentParser()
    xdg = os.environ.get("XDG_DATA_HOME") or os.path.join(ROOT, "userdata")
    ap.add_argument("--dest", default=os.path.join(xdg, "a2r4", "install"))
    ap.add_argument("--extras", default=os.path.join(ROOT, "dl", "extras"))
    ap.add_argument("--game-dir", default=os.path.join(ROOT, "iso", "Data"))
    a = ap.parse_args()
    os.makedirs(a.dest, exist_ok=True)
    done = 0
    for exe in sorted({e for e, _, _ in FILES}):
        path = os.path.join(a.extras, exe)
        if not os.path.exists(path):
            print("niet gevonden: %s (overgeslagen)" % path)
            continue
        want = {}  # sha1 -> namen (l0/l1/l2_anim.ini zijn gelijk)
        for e, n, h in FILES:
            if e == exe:
                want.setdefault(h, []).append(n)
        found = {}
        for blob in streams(open(path, "rb").read()):
            h = hashlib.sha1(blob).hexdigest()
            if h in want:
                found[h] = blob
        missing = [n for h in want if h not in found for n in want[h]]
        if missing:
            sys.exit("fout: %s mist %s (andere versie van de installer?)" % (exe, ", ".join(missing)))
        n_exe = 0
        for h, names in want.items():
            for name in names:
                # Zelfde schrijfwijze als op de CD, zodat de overlay hem verbergt.
                target = find_nocase(a.dest, name) or find_nocase(a.game_dir, name) or name
                with open(os.path.join(a.dest, target), "wb") as f:
                    f.write(found[h])
                n_exe += 1
        done += n_exe
        print("%s: %d bestanden" % (exe, n_exe))
    add_portrait(a.dest, a.game_dir)
    print("OK: %d bestanden in %s" % (done, a.dest))


def add_portrait(dest, game_dir):
    """[Portraits] 7=PLAYER_7 in game.ini van de overlay (zoals de update)."""
    name = find_nocase(dest, "game.ini")
    src = os.path.join(dest, name) if name else os.path.join(game_dir, find_nocase(game_dir, "game.ini"))
    lines = open(src, "rb").read().decode("latin1").split("\r\n")
    sec = None
    for i, line in enumerate(lines):
        t = line.strip()
        if t.startswith("["):
            if sec == "portraits":
                break
            sec = t.strip("[]").lower()
        elif sec == "portraits" and t.split("=")[0].strip() == "7":
            return  # staat er al
    else:
        i = len(lines)
    if sec != "portraits":
        print("let op: geen [Portraits] in game.ini")
        return
    j = i
    while j > 0 and not lines[j - 1].strip():
        j -= 1  # na de laatste regel van de sectie
    lines.insert(j, "7=PLAYER_7")
    target = os.path.join(dest, name or find_nocase(game_dir, "game.ini"))
    open(target, "wb").write("\r\n".join(lines).encode("latin1"))
    print("game.ini: [Portraits] 7=PLAYER_7 toegevoegd")


if __name__ == "__main__":
    main()
