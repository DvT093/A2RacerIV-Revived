#!/bin/bash
# A2 Racer IV Revived (macOS, Apple Silicon): van de CD (ISO of BIN/CUE) naar "A2 Racer IV.app".
#
#   ./setup.sh A2RACER4.bin      de CD-image (BIN/CUE, ISO kan ook)
#   ./setup.sh /pad/naar/cd      of een map met de inhoud van de CD
#   ./setup.sh                   opnieuw draaien: slaat over wat al klaar is
#   ./setup.sh --plain ...       zonder kleur en emoji
#
# Stappen: spelbestanden, Homebrew, submodules, bouwen, HD-pack, app.
# Het script gooit je image nooit weg en zet geen spelbestanden in git.
set -euo pipefail

# HD-pack (HD-textures, menu in 4K, icoon): zip bij de GitHub-release.
HD_VERSION="hd-v1"
HD_URL="https://github.com/DvT093/A2RacerIV-Revived/releases/download/hd-v1/a2racer4-hd-v1.zip"
HD_SHA256="609ca35b46ecc90e6b9d07313ef6923de2bb3f278e0e5656aba2f954c2dfff3c"
# spel.dat is gelijk in v1.00 en v1.01 van de CD; alleen die werkt.
SPEL_SHA256="ccd1b03472b25c8b7acacf4ba7e6abf60f657dfff2efd90f29e740b03f62286b"
# De extra's die Davilex in december 2000 als download uitbracht (een truck
# en een bonustrack), uit het Wayback-archief van davilex.nl.
EXTRAS=(
  "a2racer4_update_car.exe|8e3fb0bf1f276757ad5795c6bc9068e4de3491af|https://web.archive.org/web/20010523195338id_/http://www.davilex.nl:80/service/a2racer4/download/a2racer4_update_car.exe"
  "a2racer4_update_track.exe|c45fe666d266bd6438b124889f8f6706c74ab289|https://web.archive.org/web/20010724042441id_/http://www.davilex.nl:80/service/a2racer4/download/a2racer4_update_track.exe"
)
# Wat de build nodig heeft (Homebrew-namen).
BREW_DEPS="cmake ninja pkgconf sdl3 ffmpeg freetype glslang spirv-cross llvm@22 python@3.14"
# Plus Pillow voor python3 (brew install pillow), gecheckt met een import.

# Overschrijven kan (testen, eigen mirror): A2R4_HD_URL (ook file://...),
# A2R4_HD_SHA256, A2R4_JOBS (parallelle compileertaken, standaard 2).
HD_URL="${A2R4_HD_URL:-$HD_URL}"
HD_SHA256="${A2R4_HD_SHA256:-$HD_SHA256}"
JOBS="${A2R4_JOBS:-2}"

PLAIN=0
SRC=""
for a in "$@"; do
  case "$a" in
    --plain) PLAIN=1 ;;
    -h|--help) sed -n '2,10p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) SRC="$a" ;;
  esac
done
# Kleur alleen in een echte terminal; emoji wel (ook in Claude Code).
COLOR=1
[ -t 1 ] || COLOR=0
[ "$PLAIN" = 1 ] && COLOR=0

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"
START=$(date +%s)

if [ "$COLOR" = 0 ]; then
  B=""; G=""; Y=""; R=""; D=""; N=""
else
  B=$'\033[1m'; G=$'\033[32m'; Y=$'\033[33m'; R=$'\033[31m'; D=$'\033[2m'; N=$'\033[0m'
fi
# e <emoji> <tekst>: met --plain zonder emoji.
e() { if [ "$PLAIN" = 1 ]; then printf '%s\n' "$2"; else printf '%s %s\n' "$1" "$2"; fi; }
step() { echo; e "$1" "${B}[$2/6] $3${N} ${D}($4)${N}"; }
ok() { e "  ✅" "${G}$1${N}"; }
skip() { e "  ⏭️ " "${D}$1${N}"; }
info() { e "  👉" "$1"; }
warn() { e "  ⚠️ " "${Y}$1${N}"; }
fail() {
  echo
  e "❌" "${R}${B}$1${N}"
  shift
  for l in "$@"; do e "  🔧" "$l"; done
  exit 1
}
sha() { shasum -a 256 "$1" | cut -d' ' -f1; }

echo
e "🏖️ 🚘" "${B}A2 Racer IV Revived · macOS (Apple Silicon)${N}"
e "  " "${D}Van de CD naar een app met HD-textures en een menu in 4K.${N}"

# ---------------------------------------------------------------------------
step "💿" 1 "Spelbestanden van de CD" "1 min"
have_game() { [ -f iso/Data/spel.dat ] && [ "$(sha iso/Data/spel.dat)" = "$SPEL_SHA256" ]; }
if have_game; then
  skip "iso/ is er al en klopt"
else
  if [ -z "$SRC" ]; then
    fail "Geen spelbestanden gevonden." \
      "Geef de CD-image mee: ./setup.sh ~/Downloads/A2RACER4.bin" \
      "Of een ISO, of een map met de inhoud van de CD: ./setup.sh /Volumes/A2RACER"
  fi
  [ -e "$SRC" ] || fail "Niet gevonden: $SRC" "Check het pad (sleep het bestand in Terminal om het pad te plakken)."
  MNT=""
  TMPISO=""
  cleanup() {
    if [ -n "$MNT" ]; then hdiutil detach -quiet "$MNT" 2>/dev/null || hdiutil detach -force -quiet "$MNT" 2>/dev/null || true; rmdir "$MNT" 2>/dev/null || true; fi
    [ -n "$TMPISO" ] && rm -f "$TMPISO"
    return 0
  }
  trap cleanup EXIT
  IMG="$SRC"
  case "$SRC" in *.cue|*.CUE) IMG="${SRC%.*}.bin"; [ -f "$IMG" ] || IMG="${SRC%.*}.BIN" ;; esac
  if [ -f "$IMG" ]; then
    # BIN met sectoren van 2352 bytes (MODE1/2352): de 2048 databytes per
    # sector eruit, dan is het een gewone ISO die macOS kan openen.
    if [ $(( $(stat -f %z "$IMG") % 2352 )) = 0 ] && [ "$(dd if="$IMG" bs=1 count=12 2>/dev/null | xxd -p)" = "00ffffffffffffffffffff00" ]; then
      info "CD-image omzetten naar ISO (BIN, 2352 bytes per sector)..."
      mkdir -p build
      TMPISO="$ROOT/build/cd-tmp.iso"
      python3 -I - "$IMG" "$TMPISO" <<'PY' || fail "Omzetten mislukt." "Is dit echt de BIN van de CD?"
import sys
with open(sys.argv[1], "rb") as f, open(sys.argv[2], "wb") as o:
    while True:
        s = f.read(2352 * 512)
        if not s:
            break
        o.write(b"".join(s[i + 16:i + 16 + 2048] for i in range(0, len(s), 2352)))
PY
      IMG="$TMPISO"
    fi
    MNT="$(mktemp -d /tmp/a2r4-cd.XXXXXX)"
    info "CD openen (alleen lezen): $(basename "$SRC")"
    hdiutil attach -readonly -nobrowse -noverify -mountpoint "$MNT" "$IMG" >/dev/null \
      || fail "Deze image kan ik niet openen." "Is het echt de CD (.bin, .cue of .iso)? Probeer hem eerst in Finder te openen."
    CD="$MNT"
  else
    CD="$SRC"
  fi
  # De CD-root heeft Data/spel.dat; een map die zelf de data-map is mag ook.
  FROM=""
  for d in Data data DATA; do [ -f "$CD/$d/spel.dat" ] && { FROM="$CD"; TO="iso"; DATA="$d"; break; }; done
  if [ -z "$FROM" ]; then
    if [ -f "$CD/spel.dat" ]; then FROM="$CD"; TO="iso/Data"; DATA=""
    else fail "Op deze CD staat geen Data/spel.dat." "Dit lijkt niet de CD van A2 Racer IV (Davilex, 2000)."; fi
  fi
  info "Kopiëren naar iso/ (ca. 300 MB)..."
  mkdir -p "$TO"
  rsync -a --exclude '*.iso' --exclude '*.bin' --exclude '.DS_Store' "$FROM/" "$TO/"
  [ -n "${DATA:-}" ] && [ "$DATA" != Data ] && mv "iso/$DATA" iso/Data
  chmod -R u+rwX iso
  cleanup; MNT=""; TMPISO=""; trap - EXIT
  [ -f iso/Data/spel.dat ] || fail "Kopiëren mislukt: iso/Data/spel.dat ontbreekt."
  if [ "$(sha iso/Data/spel.dat)" != "$SPEL_SHA256" ]; then
    fail "Dit is een andere versie van de CD: spel.dat klopt niet." \
      "Alleen A2 Racer IV - De politie slaat terug! v1.00 of v1.01 werkt; de port kent de adressen in die exe." \
      "Gooi iso/ weg (rm -rf iso) en probeer het met de goede CD."
  fi
  ok "Spelbestanden staan in iso/ (spel.dat klopt)"
fi

# ---------------------------------------------------------------------------
step "🍺" 2 "Gereedschap (Homebrew)" "een paar seconden"
[ "$(uname -s)" = Darwin ] || fail "Dit werkt alleen op macOS."
[ "$(uname -m)" = arm64 ] || fail "Dit werkt alleen op een Mac met Apple Silicon (M1 of nieuwer)."
if [ ! -d /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk ]; then
  fail "De Command Line Tools van Apple ontbreken." "Plak dit in Terminal: xcode-select --install"
fi
command -v brew >/dev/null 2>&1 || fail "Homebrew ontbreekt." \
  'Installeer het via https://brew.sh (één regel in Terminal), daarna opnieuw ./setup.sh'
MISSING=""
for f in $BREW_DEPS; do
  brew list --versions "$f" >/dev/null 2>&1 || MISSING="$MISSING $f"
done
if [ -n "$MISSING" ]; then
  fail "Er mist gereedschap:$MISSING" "Plak dit in Terminal: brew install$MISSING" "Daarna opnieuw: ./setup.sh"
fi
PY="$(brew --prefix)/bin/python3"
[ -x "$PY" ] || PY="$(command -v python3)"
"$PY" -c "import PIL" 2>/dev/null || fail "Er mist gereedschap: Pillow (plaatjes in Python)." "Plak dit in Terminal: brew install pillow" "Daarna opnieuw: ./setup.sh"
# make_app.sh en de tools roepen 'python3' aan: die van Homebrew vooraan.
export PATH="$(brew --prefix)/bin:$PATH"
ok "Alles aanwezig"

# ---------------------------------------------------------------------------
step "📦" 3 "SwARc en Davilex-RED (submodules)" "1 min"
# Altijd bijwerken: na een git pull wijzen de submodules naar nieuwe commits,
# en zonder deze stap zou de build de oude SwARc en Davilex-RED gebruiken.
if [ -d .git ] || [ -f .git ]; then
  git submodule update --init --recursive || fail "Submodules ophalen of bijwerken mislukt." \
    "Check je internet en probeer: git submodule update --init --recursive"
  ok "external/SwARc en external/Davilex-RED zijn bij"
elif [ -f external/SwARc/CMakeLists.txt ] && [ -f external/Davilex-RED/menu/davi_menu.cmake ]; then
  skip "external/ is er al (geen git-map, dus niet bijgewerkt)"
else
  fail "Dit is geen git-map, dus de submodules ontbreken." \
    "Haal het project op met: git clone --recursive https://github.com/DvT093/A2RacerIV-Revived.git"
fi

# ---------------------------------------------------------------------------
step "🔧" 4 "Spelcode omzetten en bouwen" "15-30 min, de eerste keer"
BUILD=out/build/release
LIFT="$BUILD/swarc/tools/lifter_x86/recomp_lift"
# CMakeLists.txt kiest zelf external/SwARc en external/Davilex-RED.
CFG=(--preset release)
if [ ! -f "$BUILD/CMakeCache.txt" ]; then
  info "Configureren (haalt ook een paar kleine bibliotheken op)..."
  cmake "${CFG[@]}" > out_configure.log 2>&1 || fail "Configureren mislukt." "Zie out_configure.log (onderaan staat de fout)."
fi
if [ ! -x "$LIFT" ]; then
  info "De lifter bouwen..."
  cmake --build "$BUILD" --target recomp_lift -j "$JOBS" > out_lifter_build.log 2>&1 \
    || fail "De lifter bouwen mislukt." "Zie out_lifter_build.log."
fi
# Liften: x86-code uit spel.dat naar C++ in generated/ (opnieuw als de
# symbolen, het manifest of de lifter nieuwer zijn).
if [ ! -f generated/.lifted ] || [ symbols/functions.tsv -nt generated/.lifted ] \
   || [ re/jumptables_manual.tsv -nt generated/.lifted ] || [ lifter.manifest -nt generated/.lifted ] \
   || [ "$LIFT" -nt generated/.lifted ]; then
  info "spel.dat omzetten naar C++ (liften)..."
  ./lift.sh > /dev/null 2>&1 || fail "Liften mislukt." "Zie out_lift.log."
  touch generated/.lifted
  cmake "${CFG[@]}" > out_configure.log 2>&1 || fail "Configureren mislukt." "Zie out_configure.log."
else
  skip "Spelcode is al omgezet"
fi
info "Compileren (a2r4 en a2r4-menu, $JOBS tegelijk)..."
cmake --build "$BUILD" --target a2r4 a2r4-menu -j "$JOBS" > out_build.log 2>&1 || {
  grep -m5 -B2 -A6 "error:" out_build.log || tail -20 out_build.log
  fail "Bouwen mislukt." "Zie out_build.log."
}
[ -x "$BUILD/a2r4" ] && [ -x "$BUILD/a2r4-menu" ] || fail "Bouwen gaf geen a2r4 of a2r4-menu." "Zie out_build.log."
ok "a2r4 (de race) en a2r4-menu (het menu) zijn gebouwd"

# ---------------------------------------------------------------------------
step "🎨" 5 "HD-pack: HD-textures en menu in 4K" "2-5 min downloaden (ca. 1,2 GB)"
if [ -f .hd-pack ] && [ "$(cat .hd-pack)" = "$HD_VERSION" ]; then
  skip "HD-pack $HD_VERSION staat er al"
else
  ZIP=build/release/a2racer4-hd-v1.zip
  mkdir -p build/release
  GOT=0
  if [ -f "$ZIP" ] && [ "$(sha "$ZIP")" = "$HD_SHA256" ]; then
    GOT=1
  else
    info "Downloaden: $HD_URL"
    rm -f "$ZIP.part"
    if curl -fL -# -o "$ZIP.part" "$HD_URL"; then
      if [ "$(sha "$ZIP.part")" = "$HD_SHA256" ]; then
        mv "$ZIP.part" "$ZIP"; GOT=1
      else
        warn "De download klopt niet (sha256 anders)."
      fi
    fi
    rm -f "$ZIP.part"
  fi
  if [ "$GOT" = 1 ]; then
    info "Uitpakken..."
    unzip -oq "$ZIP" -d "$ROOT" || fail "Uitpakken mislukt." "Gooi $ZIP weg en draai ./setup.sh opnieuw."
    echo "$HD_VERSION" > .hd-pack
    ok "HD-textures (build/texture_pack) en menu in 4K (mods/menu/local/hd) staan klaar"
  else
    warn "Het HD-pack kon ik niet ophalen. Geen probleem: ik maak de plaatjes zelf."
    warn "Dat gaat met Real-ESRGAN op je eigen Mac en duurt langer (30-60 min)."
  fi
fi

# ---------------------------------------------------------------------------
step "🏁" 6 "De extra's van Davilex en A2 Racer IV.app" "$([ -f .hd-pack ] && echo "een halve minuut" || echo "30-60 min: plaatjes maken")"
mkdir -p dl/extras
for x in "${EXTRAS[@]}"; do
  IFS='|' read -r name sum url <<< "$x"
  f="dl/extras/$name"
  if [ -f "$f" ] && [ "$(shasum "$f" | cut -d' ' -f1)" = "$sum" ]; then continue; fi
  info "Ophalen uit het Wayback-archief: $name"
  if curl -fsL -o "$f.part" "$url" && [ "$(shasum "$f.part" | cut -d' ' -f1)" = "$sum" ]; then
    mv "$f.part" "$f"
  else
    rm -f "$f.part"
    warn "$name kon ik niet ophalen; het spel werkt ook zonder (zonder truck of bonustrack)."
  fi
done
tools/make_app.sh > out_make_app.log 2>&1 || fail "De app maken mislukt." "Zie out_make_app.log."
[ -x "A2 Racer IV.app/Contents/MacOS/A2Racer4" ] || fail "De app is er niet." "Zie out_make_app.log."
ok "\"A2 Racer IV.app\" staat in $ROOT"
[ -f dl/extras/a2racer4_update_car.exe ] && ok "De truck van De Schaduw en de bonustrack staan erin"

MIN=$(( ($(date +%s) - START + 59) / 60 ))
echo
e "🎉" "${G}${B}Klaar in $MIN min! Dubbelklik op \"A2 Racer IV.app\" (of: open \"A2 Racer IV.app\").${N}"
e "  🎮" "In de race: pijltjes sturen, Cmd = turbo, F9 = origineel/HD, P = pauze, Esc = terug naar het menu."
e "  🚚" "De truck en de bonustrack vind je in Quick Race."
if [ -n "$SRC" ] && [ -f "$SRC" ]; then
  e "  💾" "$(basename "$SRC") heb je niet meer nodig, bewaar hem als back-up of gooi hem weg."
fi
echo
