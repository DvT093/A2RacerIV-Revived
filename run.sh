#!/bin/sh
# Bouwt (incrementeel) en start A2 Racer IV native. Log in run.log.
cd "$(dirname "$0")"
# Speldata (gewijzigde game.ini, saves, crashrapporten) in het project.
export XDG_DATA_HOME="$PWD/userdata"
cmake --build --preset release > out_build.log 2>&1 || { grep -m5 -B2 -A6 "error:" out_build.log; exit 1; }
timeout "${A2R4_TIMEOUT:-40}" out/build/release/a2r4 --game-dir "$PWD/iso/Data" "$@" > run.log 2>&1
echo "exit $?" >> run.log
