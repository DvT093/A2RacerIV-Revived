#!/usr/bin/env python3
"""HD-plaatjes voor het menu van A2 Racer IV, uit je eigen CD.

Elk plaatje uit MenuData/MenuRcs (400 TGA's met eigen alpha, 66 BMP's)
wordt 4x zo groot gemaakt met Real-ESRGAN (realesrgan-x4plus, het model
dat Vakantie Racer voor menuplaatjes koos) en als PNG in
mods/menu/local/hd/ gezet (gitignored). De menu-engine
(../Davilex-RED/menu/engine/hd.h) zoekt ze op de bestandsnaam zonder
extensie ("mainscre", "butp1_up") en tekent ze in plaats van het origineel
zolang Upgraded aan staat (F9).

- TGA met alpha: doorzichtige pixels krijgen eerst de kleur van hun buren
  (anders donkere of roze randen), kleur en alpha gaan apart door het model
  en worden weer samengevoegd. De alpha blijft zacht (schaduwen, gloed).
  Een TGA met overal alpha 0 tekent het menu ondoorzichtig: hier ook.
- Knopstanden: de engine zoekt een knopstand op de naam van zijn
  definitie ("gdfBtnOkDlg.UpLink"), niet op het bestand ("okay_up"). De
  koppeling staat in src/menu/a2r4/skin.cpp (DefPrefixes); per stand komt
  er een symlink met de definitienaam naast.
- BMP: <naam>.png zoals hij is, plus <naam>.alpha.png met de pixel
  linksonder als doorzichtige kleur (Delphi), voor als het menu hem met
  sleutelkleur tekent.

Alleen wat ontbreekt of veranderd is wordt opnieuw gemaakt.

Gebruik: python3 -I tools/upscale_menu.py [--only naam,naam] [--force]
"""

import argparse
import bisect
import hashlib
import io
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile

from PIL import Image, ImageChops, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VERSION = "1"
MODEL = "realesrgan-x4plus"
SCALE = 4
PAD = 8
TOOL = os.path.join(ROOT, "build", "tools", "realesrgan-ncnn-vulkan")


def find_nocase(d, name):
    for f in os.listdir(d):
        if f.lower() == name.lower():
            return os.path.join(d, f)
    return None


def archive(d):
    """MenuRcs.IND/.IMG: u16 aantal, dan naam[20] + u32 offset."""
    ind = open(find_nocase(d, "MenuRcs.IND"), "rb").read()
    img = open(find_nocase(d, "MenuRcs.IMG"), "rb").read()
    n = struct.unpack("<H", ind[:2])[0]
    ents = [(ind[2 + 24 * i:22 + 24 * i].split(b"\0")[0].decode("latin1"),
             struct.unpack("<I", ind[22 + 24 * i:26 + 24 * i])[0]) for i in range(n)]
    offs = sorted({o for _, o in ents} | {len(img)})
    for name, off in ents:
        yield name, img[off:offs[bisect.bisect_right(offs, off)]]


def fill_transparent(rgb, alpha):
    """Pixels met alpha 0 krijgen de kleur van hun zichtbare buren."""
    mask = alpha.point(lambda v: 255 if v > 0 else 0)
    rgba = rgb.convert("RGBA")
    rgba.putalpha(mask)
    out = rgb.copy()
    known = mask.copy()
    for radius in (1, 2, 4, 8, 16, 32, 64, 128):
        blurred = rgba.convert("RGBa").filter(ImageFilter.BoxBlur(radius)).convert("RGBA")
        a = blurred.getchannel("A").point(lambda v: 255 if v > 0 else 0)
        out.paste(blurred.convert("RGB"), (0, 0), ImageChops.subtract(a, known))
        known = ImageChops.lighter(known, a)
        if known.getextrema()[0] == 255:
            break
    return out


def pad(im):
    w, h = im.size
    out = Image.new(im.mode, (w + 2 * PAD, h + 2 * PAD))
    out.paste(im.resize((w + 2 * PAD, h + 2 * PAD), Image.Resampling.NEAREST), (0, 0))
    out.paste(im, (PAD, PAD))
    return out


def unpad(im, w, h):
    return im.crop((PAD * SCALE, PAD * SCALE, (PAD + w) * SCALE, (PAD + h) * SCALE))


def jobs(game_dir):
    """(naam, uitvoer, rgb, alpha of None) per te maken PNG."""
    items = list(archive(os.path.join(game_dir, "MenuData")))
    tgas = {n.lower()[:-4] for n, _ in items if n.lower().endswith(".tga")}
    for name, data in items:
        low = name.lower()
        # HD-namen gaan zonder extensie: bij radio_up.bmp naast radio_up.tga
        # wint de TGA (die tekent de knop).
        if low.endswith(".bmp") and low[:-4] in tgas:
            continue
        base = name.rsplit(".", 1)[0]
        if not (low.endswith(".tga") or low.endswith(".bmp")) or not data:
            continue
        try:
            im = Image.open(io.BytesIO(data))
            im.load()
        except Exception:
            continue
        if low.endswith(".tga"):
            im = im.convert("RGBA")
            a = im.getchannel("A")
            if a.getextrema()[1] == 0:
                yield base, base, im.convert("RGB"), None  # overal 0: ondoorzichtig
            else:
                yield base, base, im.convert("RGB"), a
        else:
            rgb = im.convert("RGB")
            yield base, base, rgb, None
            key = rgb.getpixel((0, rgb.size[1] - 1))
            m = Image.eval(ImageChops.difference(rgb, Image.new("RGB", rgb.size, key)).convert("L"),
                           lambda v: 255 if v else 0)
            yield base, base + ".alpha", rgb, m


LINKS = ("UpLink", "DownLink", "FocusedLink", "HoveredLink", "DisabledLink")
SUFFIX = ("up", "do", "fo", "hi", "di")


def link_button_states(out_dir):
    """<definitie>.<stand>.png -> <prefix>_<up|do|..>.png (skin.cpp DefPrefixes)."""
    import re
    src = open(os.path.join(ROOT, "src", "menu", "a2r4", "skin.cpp")).read()
    n = 0
    for name, prefix in re.findall(r'\{"T\w+\.(\w+)",\s*"(\w+)"\}', src):
        for link, suf in zip(LINKS, SUFFIX):
            target = "%s_%s.png" % (prefix, suf)
            path = os.path.join(out_dir, "%s.%s.png" % (name, link))
            if not os.path.exists(os.path.join(out_dir, target)):
                continue
            if os.path.lexists(path):
                os.remove(path)
            os.symlink(target, path)
            n += 1
    return n


ESRGAN_ZIP = ("https://github.com/xinntao/Real-ESRGAN/releases/download/v0.2.5.0/"
              "realesrgan-ncnn-vulkan-20220424-macos.zip",
              "e0ad05580abfeb25f8d8fb55aaf7bedf552c375b5b4d9bd3c8d59764d2cc333a")


def fetch_tool():
    """Real-ESRGAN met zijn eigen modellen (realesrgan-x4plus), als hij ontbreekt."""
    if os.path.exists(os.path.join(TOOL, "realesrgan-ncnn-vulkan")):
        return
    import urllib.request
    import zipfile
    url, digest = ESRGAN_ZIP
    os.makedirs(TOOL, exist_ok=True)
    z = os.path.join(TOOL, "realesrgan.zip")
    print("ophalen:", url)
    with urllib.request.urlopen(url) as r, open(z, "wb") as f:
        shutil.copyfileobj(r, f)
    if hashlib.sha256(open(z, "rb").read()).hexdigest() != digest:
        os.remove(z)
        sys.exit("fout: %s heeft een andere sha256 dan verwacht" % url)
    with zipfile.ZipFile(z) as zf:
        for n in zf.namelist():
            if n.endswith("/") or not (n == "realesrgan-ncnn-vulkan" or n.startswith("models/")):
                continue
            out = os.path.join(TOOL, n)
            os.makedirs(os.path.dirname(out), exist_ok=True)
            with zf.open(n) as src, open(out, "wb") as dst:
                shutil.copyfileobj(src, dst)
    os.chmod(os.path.join(TOOL, "realesrgan-ncnn-vulkan"), 0o755)
    os.remove(z)


def upscale(src, dst):
    r = subprocess.run(["./realesrgan-ncnn-vulkan", "-i", src, "-o", dst, "-n", MODEL, "-s", str(SCALE),
                        "-m", "models", "-f", "png"],
                       cwd=TOOL, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    if r.returncode:
        sys.exit("fout: realesrgan-ncnn-vulkan (%d):\n%s" % (r.returncode, r.stderr[-2000:]))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--game-dir", default=os.path.join(ROOT, "iso", "Data"))
    ap.add_argument("--out", default=os.path.join(ROOT, "mods", "menu", "local", "hd"))
    ap.add_argument("--only", help="alleen deze namen (komma's, zonder extensie)")
    ap.add_argument("--force", action="store_true")
    a = ap.parse_args()
    if os.environ.get("A2R4_SKIP_UPSCALE"):
        print("HD-menu overgeslagen (A2R4_SKIP_UPSCALE)")
        return
    only = {n.strip().lower() for n in a.only.split(",")} if a.only else None
    os.makedirs(a.out, exist_ok=True)
    stamps_path = os.path.join(a.out, ".stamps.json")
    try:
        stamps = json.load(open(stamps_path))
    except (OSError, ValueError):
        stamps = {}
    todo = []
    for name, out, rgb, alpha in jobs(a.game_dir):
        if only and name.lower() not in only:
            continue
        h = hashlib.sha1(rgb.tobytes() + (alpha.tobytes() if alpha else b"") + VERSION.encode()).hexdigest()
        if a.force or stamps.get(out) != h or not os.path.exists(os.path.join(a.out, out + ".png")):
            todo.append((out, rgb, alpha, h))
    print("menu: %d plaatjes te maken" % len(todo))
    if not todo:
        print("knopstanden: %d namen" % link_button_states(a.out))
        return
    fetch_tool()
    with tempfile.TemporaryDirectory(dir=os.path.join(ROOT, "build")) as tmp:
        src, dst = os.path.join(tmp, "in"), os.path.join(tmp, "out")
        os.makedirs(src)
        os.makedirs(dst)
        for i, (out, rgb, alpha, h) in enumerate(todo):
            col = fill_transparent(rgb, alpha) if alpha else rgb
            pad(col).save(os.path.join(src, "%04d.png" % i))
            if alpha:
                pad(alpha.convert("RGB")).save(os.path.join(src, "%04d_a.png" % i))
        upscale(src, dst)
        for i, (out, rgb, alpha, h) in enumerate(todo):
            w, ht = rgb.size
            big = unpad(Image.open(os.path.join(dst, "%04d.png" % i)).convert("RGB"), w, ht)
            if alpha:
                al = unpad(Image.open(os.path.join(dst, "%04d_a.png" % i)).convert("L"), w, ht)
                # Het model laat een waas van bijna-nul alpha achter (een vaag
                # vierkant om kleine knoppen): onder 16 is doorzichtig.
                al = al.point(lambda v: 0 if v < 16 else v)
                hist = alpha.histogram()
                if not any(hist[1:255]):
                    al = al.point(lambda v: 255 if v >= 128 else 0)  # harde sleutel: harde rand
                big = big.convert("RGBA")
                big.putalpha(al)
            big.save(os.path.join(a.out, out + ".png"), compress_level=6)
            stamps[out] = h
    json.dump(stamps, open(stamps_path, "w"), indent=0)
    print("knopstanden: %d namen" % link_button_states(a.out))
    print("OK: %d plaatjes in %s" % (len(todo), os.path.relpath(a.out, ROOT)))


if __name__ == "__main__":
    main()
