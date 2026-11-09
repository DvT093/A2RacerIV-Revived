// A2 Racer IV menu: game data and shared state of the flow.
// Everything comes from the game's own ini files (MenuData/static.ini,
// menu.ini, MenuCaps.ini, Agent.ini) with the mod layer (mods/menu/) on top.
#pragma once

#include <map>
#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

#include "engine/app.h"
#include "launcher/race_launcher.h"

namespace a2 {

// static.ini [charN]
struct Character {
  int index = 0;  // N of [charN]
  std::string name, story, winracetext, lockedtext;
  int indexingame = 0;  // game.ini [PLAYER] car
  int speed = 0, accell = 0, handling = 0, brakes = 0;
};

// static.ini [TRACKn]
struct Track {
  int index = 0;
  std::string name, type, story, wintext, qualifytext, losetext;
  int nrlaps = 1, indexingame = 0;
  std::vector<int> qualify;    // QUALIFYPOSITION per difficulty (0 = easy)
  std::vector<int> chasetime;  // CHASETIME per difficulty (ms, 0 = no chase)
  bool british = false;
  bool IsChase() const;
};

// static.ini [TUNEUPS] SPECSn / TEXTn
struct TuneUp {
  std::string name, text;
  int price = 0, dspeed = 0, dacc = 0, dbrakes = 0, dhandl = 0;
};

// Per character progress in the Tour (TCharResult: +8 money, +0xc track
// to drive, +0x10 turbo, +0x14.. tune-ups).
struct CharResult {
  int tracktodrive = 0;  // index into the Tour order ([TOURRACE] TRACKIX)
  int money = 0;         // Euro
  int turbo = 100;       // 0-100
  bool tuneup[5] = {};   // [TUNEUPS] SPECS0-3 bought for the next race (4 = turbo oil)
  bool won = false;      // won the Tour with this character
};

struct Player {
  std::string name;
  int difficulty = 1;  // 0 easy, 1 normal, 2 hard
  bool show_start_info = true;
  int music_volume = 50, fx_volume = 50, voices_volume = 100;
  int resolution = 3, device = 0;
  bool video3d = true;  // Opties > Weergave "3D kaart" (chkVideo3d)  // 0 keyboard, 1 joypad, 2 joystick, 3 wheel
  int keys[7] = {38, 40, 37, 39, 17, 18, 32};  // up down left right extra1-3 (VK)
  int joy_sensitivity = 50, wheel_sensitivity = 50;
  int gamma = 0;
  bool alpha = true, bilinear = true, waitvbl = false;
  // Quick Race
  int qr_char = 1, qr_track = 0;
  // Tour
  int current_char = -1;  // index into [TOURRACE] CHARIX, -1 = not chosen yet
  int track_shown = 0;    // Tour position shown on the Track screen (can be an earlier one)
  bool supercop = false;  // D.T. Harry unlocked (Quick Race)
  bool supercop_new = false;  // unlocked by the last Tour win, not shown yet
  std::vector<CharResult> results;  // one per Tour character
  CharResult& Res(int c) {
    if (c < 0) c = 0;
    if (c >= int(results.size())) results.resize(c + 1);
    return results[c];
  }
};

class Game;

// Base of the A2 Racer IV screens.
class A2Screen : public davi::Screen {
 public:
  A2Screen(Game& g, const std::string& cls);
  Game& game;
  void Update(double dt) final;
  void PaintOver(SDL_Surface* canvas, int ox, int oy) final;
  void OnClick(davi::Widget& w) final;
  bool OnKey(SDL_Keycode k) override;
  // For the screens.
  virtual void Clicked(davi::Widget& w) { (void)w; }
  virtual void Tick(double dt) { (void)dt; }
  virtual void Paint(SDL_Surface* canvas, int ox, int oy) { (void)canvas; (void)ox; (void)oy; }
  // TBaseFormOneWay / TwoWay: btnNext and btnPrevious.
  virtual void Next() {}
  virtual void Previous();
  // TGameMenuScreen.btnHelpClick: Hunny explains the screen (Agent.ini).
  void Help();
};

class Game {
 public:
  Game(davi::App& app, red::Install install, std::vector<std::string> race_argv);
  bool Load();

  davi::App& app;
  davi::Assets& assets;
  red::RaceLauncher launcher;
  davi::LayeredIni stat;   // static.ini
  davi::LayeredIni menu;   // menu.ini
  davi::LayeredIni agent;  // Agent.ini (Hunny's help texts)
  std::vector<Character> chars;  // by [charN] index (gaps empty)
  std::vector<Track> tracks;     // by [TRACKn] index
  std::vector<TuneUp> tuneups;
  std::vector<int> qr_tracks, qr_chars, tour_tracks, tour_chars;
  int qr_locked_char = -1, qr_download_char = -1;
  // FDownloadCharIsAvailable (Menu.exe 0x48e9f8): L0_ANIM.INI has the
  // section static.ini [QUICKRACE] DOWNLOADSECTION with Count > 0, which
  // Davilex's car update (the truck, tools/install_extras.py) adds.
  bool qr_download_available = false;
  std::vector<Player> players;
  int current_player = -1;
  std::map<std::string, davi::Sound> embedded;  // TData sounds ("__wavBtnDown")
  // Hunny's voice lines (TData.AgentSpeak 0x480584): name + "_" + random
  // 00..n-1; AgentSpeakIdx 0x4806b0: name + %02d(idx) + "_" + random.
  void Speak(const std::string& name, int n);
  void SpeakIdx(const std::string& name, int idx, int n);

  // 1 = Quick Race, 2 = Tour (Data+0xa8; menu.ini PREVSTATE after a race).
  int mode = 0;
  red::RaceResult last;
  // After a race (Data+0xd0): RR_CHASE 1, RR_QUALIFY 2, RR_WINRACE 4 (Tour
  // won), RR_WINTRACK 8, RR_BESTLAP 32.
  int rrflags = 0;
  struct Ranked { int ch = -1; int time_ms = 0; };  // ch = [charN] index
  std::vector<Ranked> ranking;
  int playerpos = 0;
  int race_track = 0;  // [TRACKn] index of the last race
  // menu.ini [HIGHSCORES]: best lap per [TRACKn].
  struct Record { int time_ms = 0; std::string name; };
  std::vector<Record> records;
  void SaveRecords();
  // The character / track the player races with now.
  int QrChar() const;    // [charN] index of the Quick Race choice
  int QrTrack() const;   // [TRACKn] index
  int TourChar() const;  // [charN] index of the Tour character (-1 none)
  int TourTrack() const; // [TRACKn] index of the shown Tour track
  CharResult* TourRes();
  int CharByIndexInGame(int car) const;

  Player* Cur() { return current_player >= 0 && current_player < int(players.size()) ? &players[current_player] : nullptr; }
  Character* Char(int idx) { return idx >= 0 && idx < int(chars.size()) && !chars[idx].name.empty() ? &chars[idx] : nullptr; }
  void NewPlayer(const std::string& name);
  void SavePlayers();
  std::string Str(int n) const;           // menu.ini [MENUSTRINGS]
  std::string Ordinal(int pos1) const;    // [ordinalnumbers]
  std::string FormatTime(int ms) const;   // [FORMAT] TIME
  // A wav from MenuRcs; voice lines stop the previous one.
  void Sound(const std::string& name);
  void Voice(const std::string& name);
  void StartMusic();
  SDL_Surface* Pic(const std::string& name) { return assets.Bitmap(name); }

  template <class T> T* Screen() {
    auto& p = screens_[typeid(T).name()];
    if (!p) p = std::make_unique<T>(*this);
    return static_cast<T*>(p.get());
  }
  void Goto(davi::Screen* s);
  void Back();

 private:
  void PlayerIo(davi::IniFile& f, const std::string& sec, Player& p, bool save);
  std::map<std::string, std::unique_ptr<davi::Screen>> screens_;
  std::map<davi::Screen*, davi::Screen*> prev_;
  std::string players_path_;
};

void RegisterSkin(davi::Skin& skin);
std::string Fmt(const std::string& f, const std::string& a);

}  // namespace a2
