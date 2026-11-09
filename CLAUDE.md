# A2 Racer IV - native Mac-port (decomp)

Doel: A2 Racer IV - De politie slaat terug! (Davilex, NL, v1.00 20-10-2000 / v1.01 22-12-2000) **native** op Apple Silicon, volledig moddable, 60 fps. Zelfde aanpak als `../Vakantieracer`: static recompilation met SwARc (x86 -> C++ -> clang arm64), daarna functie voor functie herschrijven naar leesbare C. Lees eerst `../Vakantieracer/CLAUDE.md`: methode, tooling en regels gelden hier ook. Dit bestand noemt alleen wat anders is.

## Doel en takenlijst

## Welke exe we porten
**`iso/Data/spel.dat`** (sha256 `ccd1b034...f62286b`). Die is **byte-voor-byte gelijk in v1.00 en v1.01**: de patch veranderde alleen `Raser.ini` (`VERSION=`), `version.txt`, en voegde splash-BMP's, `default.txt` en `A2Racer4.icd` toe. Er is dus maar één gamecode; alle adressen (`// 0x4xxxxx`) verwijzen naar deze exe.
- Onbeveiligd, PE32 MSVC, imagebase 0x400000, entry 0x559715 (RVA 0x159715). `.text` 1.524.810 bytes (VR: kleiner).
- `iso/` = de v1.01-CD (data-bestanden zijn de nieuwste). De v1.00-ISO staat nog in `dl/`.

## Platformoppervlak
142 imports: precies de 137 van Vakantie Racer plus 5 console-functies (`GetConsoleMode`, `SetConsoleMode`, `PeekConsoleInputA`, `ReadConsoleInputA`, `GetNumberOfConsoleInputEvents`, waarschijnlijk alleen CRT; Europe Racer heeft ze ook). **Geen Miles**: geluid is DirectSound, net als VR. DDRAW 3, DINPUT 1, DSOUND 2. Alles wat VR in SwARc nodig heeft, dekt deze game dus ook.
- 120 originele bronpaden gelekt via asserts (`C:\Work\A2IV\Game\prog\...`), in `re/source_files.txt`. Nieuw t.o.v. VR o.a. `ai_supercop.cpp`, `pth_pcop.cpp`, `physicalworld.cpp`, `car_dmg.cpp`, `ovl_communicator.cpp` (politie-AI, schade, nieuwe natuurkunde).
- Zelfde fps-strings als Europe Racer: `vchunks: %d  FPSMULT:%f`, `frameRate: %f Real: %f`, `fps = %f`. Uitzoeken of de natuurkunde met de framerate meeschaalt vóór we op 60 fps vastzetten.

## Verschillen met Vakantie Racer (inventaris 9 nov 2026)
| Onderdeel | Vakantie Racer | A2 Racer IV |
|---|---|---|
| Game-exe | `spel.dat`, 137 imports | `spel.dat`, 142 imports (VR + 5 console) |
| Geluid | DirectSound | DirectSound (+ los archief `sfx.img/.ind`) |
| Launcher | `VRacer.exe` | `A2Racer4.exe` (36 KB) |
| Menu | `MenuData/Menu.exe`, Delphi | `MenuData/Menu.exe`, **ook Delphi**, plus **Microsoft Agent** (`Hunny.acs`, `Agent.ini`) als pratende helper, AVI's `CH00-04.avi`, eigen archief `MenuRcs.*` |
| Levels | `etappe0-5` | `etappe0-8` (9 etappes, zelfde bestandstypes) |
| Intro | - | `intro1.mpg` (MPEG-1) |

- **Archief is hetzelfde formaat**: `Data/rcs.IND` leest direct met `../Vakantieracer/re/scripts/rcs_list.py` (2.787 entries: 1.971 `.d3d`, 406 wav, 199 pcx, 173 tga, 31 fnt). Ook `RC.IMG/.IND` zoals VR.
- Nieuwe ini's: `cops.ini`, `damage.ini`, `communi.ini`, `garage.ini`, `traffic.ini`, `opponent.ini`, `overlay0-6.ini`.
- MS Agent bestaat niet op de Mac: bij het native menu de helper zelf naspelen of weglaten (keuze voor later).

## Menu (a2r4-menu)
- Native vervanger van `MenuData/Menu.exe` (Delphi 5), gebouwd 9 nov 2026. Leest bij elke start de formulieren (DFM) uit de eigen Menu.exe, de plaatjes en stemmen uit `MenuData/MenuRcs.IMG`, teksten uit `MenuCaps.ini`, `menu.ini`, `static.ini`, `Agent.ini`. Er staat geen speldata in git. Bevindingen en adressen: `re/findings.md`.
- **Gedeelde menu-engine**: engine en race-launcher staan in `../Davilex-RED/menu` (zie `MIGRATION.md` daar), samen met Vakantie Racer en Europe Racer. A2-eigen keuzes gaan via `Skin` (`a2r4/skin.cpp`: `def_link`, `raw_font_index`, `tgdi_dfm_caption = false`). Een engine-wijziging: klein, standaard het gedrag van de anderen laten, en de andere sessies melden. Nooit iets in `../Vakantieracer` of `../Europe Racer` zelf aanpassen.
- **4K:** het menu tekent op de echte schermmaat met HD-plaatjes uit `mods/menu/local/hd` (`tools/upscale_menu.py`, realesrgan-x4plus, niet in git). F9 wisselt naar het origineel. Testen: `A2R4_MENU_UPGRADES=0` (origineel), `A2R4_MENU_SIZE=3024x1964` (offscreen op die maat). Knopstanden zoekt de engine op de definitienaam (`gdfBtnOkDlg.UpLink`): het script zet die als symlink naast `okay_up.png`.
- Bouwen: `cmake --preset release` (eenmalig), dan `cmake --build --preset release --target a2r4-menu` (licht). Starten: `"A2 Racer IV.app"` (`tools/make_app.sh`; icoon via `tools/exe_icon.py`) of `XDG_DATA_HOME=$PWD/userdata out/build/release/a2r4-menu --game-dir $PWD/iso/Data --mods $PWD/mods/menu`. Opties: `--no-intro`, `--race BIN`, `--script FILE` (`<ms> click <component>`, `key <naam>`, `type <tekst>`, `shot <png>`, `quit`), `--shot FILE` (F12), `--form TfrmXxx` (één formulier kaal tonen).
- **Testen zonder zichtbaar venster** (anders pakt het venster zijn toetsenbord): `SDL_VIDEO_DRIVER=dummy SDL_RENDER_DRIVER=software`. Nep-race: `--race tools/fake_race.py` met `FAKE_RESULT=win|lose|escape|caught|abort`.
- `src/menu/a2r4/` (per origineel formulier een scherm: `screen_main/quick/tour/after.cpp`, `dialogs.cpp`, `race.cpp` = game.ini-sleutels en de flow na de race, `skin.cpp` = plaatjesprefixen).
- Spelers staan in `userdata/a2r4/menu_players.ini` (eigen formaat; geld, tune-ups en turbo per Tour-karakter). Ronderecords in de overlay-kopie van `MenuData/menu.ini` `[HIGHSCORES]`.
- Het menu start de race (`out/build/release/a2r4`) in zijn eigen venster. De race leest de `game.ini` die het menu in de overlay schrijft (`userdata/a2r4/install/`): SwARc leest eerst de overlay.

## Race (a2r4)
- `spel.dat` gelift met SwARc (eigen worktree `../SwARc-a2r4`, branch `a2r4`). `./lift.sh` (zwaar), dan `cmake --build --preset release --target a2r4`. `run.sh` bouwt en start (log in `run.log`).
- Stil testen: `SDL_AUDIO_DRIVER=dummy A2R4_BACKGROUND=1 A2R4_WINDOW=-9000,-9000,1024,768`, plus `A2R4_TEST_INPUT=file` (`3 key Up 14`, `12 key F9`), `A2R4_SHOT_EVERY=N A2R4_SHOT_DIR=map`. De dummy-videodriver kan geen Metal. `A2R4_TRACE=1` altijd door grep/tail.
- Beeld: rendermaat via 0x4C4330 (`src/game/gfx_resolution.cpp`, `A2R4_RESOLUTION=original|WxH`), 4x MSAA, 16x AF, F9/F10/F11. HD-textures: `tools/make_texture_pack.py` (4xNomos8kSC, `build/texture_pack`, 206 textures, hashes gecontroleerd met `A2R4_TEX_DUMP`). Bladen van 768x512 laadt A2 ook in stukken (links 512, kwarten rechts).
- Open: zonder invoer sluit de race na ~18 s af met "Mouse is not initialised".
- Anders dan het origineel: Hunny (MS Agent) is een plaatje met haar vragen uit `Agent.ini`, zonder de antwoorden uit `Menu.hlp`; "Geavanceerd" (videokaart) en "Instellen" (toetsen, stuur) doen nog niets; Internet toont alleen de vraag; de debug-popup (Money = 300 e.d.) ontbreekt; de spelerskaarten gebruiken `pchNN` van het Tour-karakter (het lettertype op de kaart is een gok); de tijden van WinFinal-stemmen (6 s) zijn geschat, niet gemeten.

## Drie lagen (afspraak uit Vakantie Racer, hier de derde gebruiker)
1. **Algemeen Windows/DirectX**: in SwARc, A2's eigen worktree `../SwARc-a2r4` (branch `a2r4`, afgesplitst van `legacy-dx5`). Fixes van Vakantie Racer neem je over met cherry-pick; nooit in `../SwARc-dx5` werken.
2. **RED-engine** (archief, `.d3d`, ini-structuur, Delphi-menu-onderdelen): generiek, naar `../Davilex-RED`. Tot die repo er is: importeer uit `../Vakantieracer/re/scripts/`, kopieer niet.
3. **A2 Racer IV-specifiek** (adressen in spel.dat, GameConfig, menuschermen, politie-mods): alleen hier.
Bij elke nieuwe functie: vraag eerst in welke laag hij hoort.

## Wat er is
- `Disc/` - de zip zoals gedownload (beide ISO's). Bron, niet aankomen.
- `dl/` - `v100.iso` (label `200902171648`, 405 MB) en `v101.iso` (label `A2RACER`, 306 MB).
- `iso/` - inhoud van de v1.01-CD (291 MB, gekopieerd via `hdiutil attach`). Speldata in `iso/Data/`, handleiding in `iso/Handleiding/Handleiding.pdf`.
- `re/source_files.txt` - 120 gelekte bronpaden. `re/findings.md` - menu-bevindingen. `re/ghidra/A2Menu` - Ghidra-project van Menu.exe.
- `src/menu/` - het native menu (zie Menu). `mods/menu/` - menu-mods (zelfde idee als VR).
- `re/ghidra/A2Racer4` (Ghidra-project van spel.dat), `symbols/`, `generated/` (gitignored). `re/scripts/resolve_jumptables.py` vult de sprongtabellen aan die Ghidra mist.

## Regels
- **Laptop 16 GB**: max één zware job (Ghidra `MAXMEM=2G` of een build), nooit parallel, ook niet naast een VR- of ER-build. Pauzeren bij <2 GB vrij.
- Originele data, ISO's en exe's nooit committen of publiceren.
- Elke gedecompileerde functie: originele adres in een comment (`// 0x4xxxxx`).
- Verifieer gedrag tegen het origineel (screenshots/waarden), niet op gevoel.
