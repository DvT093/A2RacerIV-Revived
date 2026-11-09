#!/bin/sh
# Maakt "A2 Racer IV.app" in de projectmap: dubbelklikken = het menu.
# Het menu start de race (a2r4) zodra die geport is; tot dan meldt het dat.
# Het icoon (build/A2Racer4.icns, uit A2Racer4.exe van je eigen CD via
# tools/exe_icon.py) staat niet in git.
set -e
cd "$(dirname "$0")/.."
APP="A2 Racer IV.app"
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
cat > "$APP/Contents/MacOS/A2Racer4" <<'SH'
#!/bin/sh
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$ROOT" || exit 1
BIN="$ROOT/out/build/release/a2r4-menu"
if [ ! -x "$BIN" ]; then
  osascript -e 'display alert "A2 Racer IV" message "Nog niet gebouwd. Bouw eerst a2r4-menu (zie CLAUDE.md)."'
  exit 1
fi
export XDG_DATA_HOME="$ROOT/userdata"
export A2R4_FULLSCREEN="${A2R4_FULLSCREEN:-window}"
exec "$BIN" --game-dir "$ROOT/iso/Data" --mods "$ROOT/mods/menu" > "$ROOT/app.log" 2>&1
SH
chmod +x "$APP/Contents/MacOS/A2Racer4"
[ -f build/A2Racer4.icns ] && cp build/A2Racer4.icns "$APP/Contents/Resources/A2Racer4.icns"
cat > "$APP/Contents/Info.plist" <<'PL'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleName</key><string>A2 Racer IV</string>
	<key>CFBundleDisplayName</key><string>A2 Racer IV</string>
	<key>CFBundleIdentifier</key><string>local.a2racer4.mac</string>
	<key>CFBundleExecutable</key><string>A2Racer4</string>
	<key>CFBundleIconFile</key><string>A2Racer4</string>
	<key>CFBundlePackageType</key><string>APPL</string>
	<key>CFBundleShortVersionString</key><string>0.1</string>
	<key>LSMinimumSystemVersion</key><string>11.0</string>
	<key>NSHighResolutionCapable</key><true/>
</dict>
</plist>
PL
codesign --force -s - "$APP" >/dev/null 2>&1 || true
touch "$APP"
echo "OK: $APP"
