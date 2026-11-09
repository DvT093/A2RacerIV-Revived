#include "a2r4/game.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <random>

namespace a2 {

using davi::Split;

namespace {

int ToInt(const std::string& s, int def = 0) {
  char* e;
  long v = std::strtol(davi::Trim(s).c_str(), &e, 10);
  return e == s.c_str() ? def : int(v);
}

std::vector<int> Ints(const std::string& s) {
  std::vector<int> v;
  for (auto& x : Split(s, ',')) v.push_back(ToInt(x));
  return v;
}

float Gain(int vol) { return std::max(0, std::min(100, vol)) / 100.0f; }

}  // namespace

std::string Fmt(const std::string& f, const std::string& a) {
  size_t p = f.find("%s");
  if (p == std::string::npos) p = f.find("%d");
  if (p == std::string::npos) return f;
  return f.substr(0, p) + a + f.substr(p + 2);
}

bool Track::IsChase() const {
  for (int t : chasetime)
    if (t) return true;
  return false;
}

Game::Game(davi::App& a, red::Install install, std::vector<std::string> race_argv)
    : app(a), assets(*a.skin.assets), launcher(std::move(install), std::move(race_argv)) {}

bool Game::Load() {
  const red::Install& in = launcher.install();
  stat.AddLayer(in.ReadPath("MenuData/static.ini"));
  menu.AddLayer(in.ReadPath("MenuData/menu.ini"));
  agent.AddLayer(in.ReadPath("MenuData/Agent.ini"));
  for (auto& d : assets.Dirs()) {  // mod layer: mods/menu/static.ini etc.
    if (d.find("MenuData") != std::string::npos) continue;
    stat.AddLayer(d + "/static.ini");
    menu.AddLayer(d + "/menu.ini");
    agent.AddLayer(d + "/Agent.ini");
  }

  int nchars = stat.GetInt("GENERAL", "CHARCOUNT", 0);
  for (int i = 0; i < std::max(nchars, 16); i++) {
    std::string s = "char" + std::to_string(i);
    if (!stat.HasSection(s)) {
      if (i >= nchars) break;
      chars.emplace_back();
      continue;
    }
    Character c;
    c.index = i;
    c.name = stat.Get(s, "name");
    c.story = stat.Get(s, "story");
    c.winracetext = stat.Get(s, "winracetext");
    c.lockedtext = stat.Get(s, "lockedtext");
    c.indexingame = stat.GetInt(s, "indexingame", i);
    c.speed = stat.GetInt(s, "speed");
    c.accell = stat.GetInt(s, "accell");
    c.handling = stat.GetInt(s, "handling");
    c.brakes = stat.GetInt(s, "brakes");
    chars.push_back(c);
  }
  int ntracks = stat.GetInt("GENERAL", "TRACKCOUNT", 0);
  for (int i = 0; i < ntracks || stat.HasSection("TRACK" + std::to_string(i)); i++) {
    std::string s = "TRACK" + std::to_string(i);
    if (!stat.HasSection(s)) break;
    Track t;
    t.index = i;
    t.name = stat.Get(s, "NAME");
    t.type = stat.Get(s, "TYPE");
    t.story = stat.Get(s, "STORY");
    t.wintext = stat.Get(s, "WINTEXT");
    t.qualifytext = stat.Get(s, "QUALIFYTEXT");
    t.losetext = stat.Get(s, "LOSETEXT");
    t.nrlaps = stat.GetInt(s, "NRLAPS", 1);
    t.indexingame = stat.GetInt(s, "INDEXINGAME", i);
    t.qualify = Ints(stat.Get(s, "QUALIFYPOSITION", "0,0,0"));
    t.chasetime = Ints(stat.Get(s, "CHASETIME", "0,0,0"));
    t.british = stat.GetInt(s, "BRITISH") != 0;
    tracks.push_back(t);
  }
  for (int i = 0; stat.Has("TUNEUPS", "SPECS" + std::to_string(i)); i++) {
    auto v = Split(stat.Get("TUNEUPS", "SPECS" + std::to_string(i)), ',');
    TuneUp t;
    if (v.size() >= 6) {
      t.name = v[0];
      t.price = ToInt(v[1]);
      t.dspeed = ToInt(v[2]);
      t.dacc = ToInt(v[3]);
      t.dbrakes = ToInt(v[4]);
      t.dhandl = ToInt(v[5]);
    }
    t.text = stat.Get("TUNEUPS", "TEXT" + std::to_string(i));
    tuneups.push_back(t);
  }
  qr_tracks = Ints(stat.Get("QUICKRACE", "TRACKIX"));
  qr_chars = Ints(stat.Get("QUICKRACE", "CHARIX"));
  tour_tracks = Ints(stat.Get("TOURRACE", "TRACKIX"));
  tour_chars = Ints(stat.Get("TOURRACE", "CHARIX"));
  qr_locked_char = stat.GetInt("QUICKRACE", "LOCKEDCHARIX", -1);
  qr_download_char = stat.GetInt("QUICKRACE", "DOWNLOADCHARIX", -1);
  {
    davi::IniFile anim;
    anim.Load(in.ReadPath("L0_ANIM.INI"));
    std::string sec = stat.Get("QUICKRACE", "DOWNLOADSECTION");
    if (sec.empty()) sec = "DOWNLOADCAR";
    qr_download_available = anim.GetInt(sec, "Count", 0) > 0;
  }

  for (size_t i = 0; i < tracks.size(); i++) {
    std::string n = std::to_string(i);
    records.push_back({menu.GetInt("HIGHSCORES", n), menu.Get("HIGHSCORES", "name" + n)});
  }
  // Players (native file; the original kept them in binary save games).
  players_path_ = in.user_dir + "/menu_players.ini";
  davi::IniFile pf;
  if (pf.Load(players_path_)) {
    int n = pf.GetInt("GENERAL", "count");
    for (int i = 0; i < n; i++) {
      Player p;
      PlayerIo(pf, "player" + std::to_string(i), p, false);
      players.push_back(p);
    }
    current_player = pf.GetInt("GENERAL", "current", -1);
    if (current_player >= int(players.size())) current_player = -1;
  }
  return !chars.empty() && !tracks.empty();
}

void Game::PlayerIo(davi::IniFile& f, const std::string& s, Player& p, bool save) {
  auto num = [&](const char* k, int& v) {
    if (save) f.SetInt(s, k, v);
    else v = f.GetInt(s, k, v);
  };
  auto flag = [&](const char* k, bool& v) {
    int i = v;
    num(k, i);
    v = i != 0;
  };
  if (save) f.Set(s, "name", p.name);
  else p.name = f.Get(s, "name");
  num("difficulty", p.difficulty);
  flag("showstartinfo", p.show_start_info);
  num("musicvolume", p.music_volume);
  num("fxvolume", p.fx_volume);
  num("voicesvolume", p.voices_volume);
  num("resolution", p.resolution);
  flag("video3d", p.video3d);
  num("device", p.device);
  static const char* keys[7] = {"up", "down", "left", "right", "extra1", "extra2", "extra3"};
  for (int k = 0; k < 7; k++) num(keys[k], p.keys[k]);
  num("joysensitivity", p.joy_sensitivity);
  num("wheelsensitivity", p.wheel_sensitivity);
  num("gamma", p.gamma);
  flag("alphablending", p.alpha);
  flag("bilinearfiltering", p.bilinear);
  flag("waitvbl", p.waitvbl);
  num("qrchar", p.qr_char);
  num("qrtrack", p.qr_track);
  num("currentchar", p.current_char);
  num("trackshown", p.track_shown);
  flag("supercop", p.supercop);
  // Per Tour character: tracktodrive,money,turbo,won,tuneup0..4
  size_t nres = save ? p.results.size() : tour_chars.size();
  for (size_t c = 0; c < nres; c++) {
    std::string k = "tour" + std::to_string(c);
    CharResult& r = p.Res(int(c));
    if (save) {
      std::string v = std::to_string(r.tracktodrive) + "," + std::to_string(r.money) + "," +
                      std::to_string(r.turbo) + "," + std::to_string(r.won);
      for (bool t : r.tuneup) v += t ? ",1" : ",0";
      f.Set(s, k, v);
    } else if (f.Has(s, k)) {
      auto v = Split(f.Get(s, k), ',');
      v.resize(9, "0");
      r.tracktodrive = ToInt(v[0]);
      r.money = ToInt(v[1]);
      r.turbo = ToInt(v[2]);
      r.won = ToInt(v[3]) != 0;
      for (int t = 0; t < 5; t++) r.tuneup[t] = ToInt(v[4 + t]) != 0;
    }
  }
}

// New player: settings from static.ini [DEFAULTSETTINGS].
void Game::NewPlayer(const std::string& name) {
  Player p;
  p.name = name;
  const char* d = "DEFAULTSETTINGS";
  p.difficulty = stat.GetInt(d, "DIFFICULTY", 1);
  p.show_start_info = stat.GetInt(d, "SHOWSTARTINFO", 1) != 0;
  p.music_volume = stat.GetInt(d, "MUSICON", 1) ? stat.GetInt(d, "MUSICVOLUME", 50) : 0;
  p.fx_volume = stat.GetInt(d, "FXON", 1) ? stat.GetInt(d, "FXVOLUME", 50) : 0;
  p.voices_volume = stat.GetInt(d, "VOICESON", 1) ? stat.GetInt(d, "VOICESVOLUME", 100) : 0;
  p.resolution = stat.GetInt(d, "RESOLUTION", 3);
  p.device = stat.GetInt(d, "CONTROLTYPE", 0);
  static const char* keys[7] = {"UP", "DOWN", "LEFT", "RIGHT", "EXTRA1", "EXTRA2", "EXTRA3"};
  for (int k = 0; k < 7; k++) p.keys[k] = stat.GetInt(d, keys[k], p.keys[k]);
  p.joy_sensitivity = stat.GetInt(d, "JOYSENSITIVITY", 50);
  p.wheel_sensitivity = stat.GetInt(d, "WHEELSENSITIVITY", 50);
  p.alpha = stat.GetInt(d, "AlphaBlending", 1) != 0;
  p.bilinear = stat.GetInt(d, "BiliniarFiltering", 1) != 0;
  p.waitvbl = stat.GetInt(d, "WaitVbl", 0) != 0;
  p.gamma = stat.GetInt(d, "Gamma", 0);
  p.qr_char = std::max(0, stat.GetInt("QUICKRACE", "DEFAULTCHAR", 0));
  p.results.resize(tour_chars.size());
  players.push_back(p);
  current_player = int(players.size()) - 1;
  SavePlayers();
}

int Game::QrChar() const {
  const Player* p = current_player >= 0 ? &players[current_player] : nullptr;
  int i = p ? p->qr_char : 0;
  return i >= 0 && i < int(qr_chars.size()) ? qr_chars[i] : (qr_chars.empty() ? 0 : qr_chars[0]);
}

int Game::QrTrack() const {
  const Player* p = current_player >= 0 ? &players[current_player] : nullptr;
  int i = p ? p->qr_track : 0;
  return i >= 0 && i < int(qr_tracks.size()) ? qr_tracks[i] : (qr_tracks.empty() ? 0 : qr_tracks[0]);
}

int Game::TourChar() const {
  const Player* p = current_player >= 0 ? &players[current_player] : nullptr;
  if (!p || p->current_char < 0 || p->current_char >= int(tour_chars.size())) return -1;
  return tour_chars[p->current_char];
}

int Game::TourTrack() const {
  const Player* p = current_player >= 0 ? &players[current_player] : nullptr;
  int i = p ? p->track_shown : 0;
  if (tour_tracks.empty()) return 0;
  return tour_tracks[std::max(0, std::min(i, int(tour_tracks.size()) - 1))];
}

CharResult* Game::TourRes() {
  Player* p = Cur();
  if (!p || p->current_char < 0) return nullptr;
  return &p->Res(p->current_char);
}

int Game::CharByIndexInGame(int car) const {
  for (auto& c : chars)
    if (!c.name.empty() && c.indexingame == car) return c.index;
  return -1;
}

void Game::SaveRecords() {
  const red::Install& in = launcher.install();
  davi::IniFile mi;
  mi.Load(in.ReadPath("MenuData/menu.ini"));
  for (size_t i = 0; i < records.size(); i++) {
    std::string n = std::to_string(i);
    mi.SetInt("HIGHSCORES", n, records[i].time_ms);
    mi.Set("HIGHSCORES", "name" + n, records[i].name);
  }
  mi.Save(in.WritePath("MenuData/menu.ini"));
}

static std::string Two(int n) {
  char b[16];
  std::snprintf(b, sizeof b, "%02d", n);
  return b;
}

void Game::Speak(const std::string& name, int n) {
  static std::mt19937 rng{std::random_device{}()};
  int r = n > 1 ? std::uniform_int_distribution<int>(0, n - 1)(rng) : 0;
  Voice(davi::Lower(name) + "_" + Two(r));
}

void Game::SpeakIdx(const std::string& name, int idx, int n) {
  static std::mt19937 rng{std::random_device{}()};
  int r = n > 1 ? std::uniform_int_distribution<int>(0, n - 1)(rng) : 0;
  Voice(davi::Lower(name) + Two(idx) + "_" + Two(r));
}

void Game::SavePlayers() {
  davi::IniFile pf;
  pf.SetInt("GENERAL", "count", int(players.size()));
  pf.SetInt("GENERAL", "current", current_player);
  for (size_t i = 0; i < players.size(); i++) PlayerIo(pf, "player" + std::to_string(i), players[i], true);
  std::error_code ec;
  std::filesystem::create_directories(std::filesystem::path(players_path_).parent_path(), ec);
  if (!pf.Save(players_path_)) SDL_Log("menu: cannot write %s", players_path_.c_str());
}

std::string Game::Str(int n) const { return menu.Get("MENUSTRINGS", std::to_string(n)); }

std::string Game::Ordinal(int pos1) const {
  std::string s = menu.Get("ordinalnumbers", std::to_string(pos1));
  return s.empty() ? std::to_string(pos1) + "e" : s;
}

std::string Game::FormatTime(int ms) const {
  std::string f = menu.Get("FORMAT", "TIME", "%d:%.2d:%.2d");
  char buf[64];
  std::snprintf(buf, sizeof buf, f.c_str(), ms / 60000, (ms / 1000) % 60, (ms / 10) % 100);
  return buf;
}

void Game::Voice(const std::string& name) {
  Player* p = Cur();
  app.audio.PlayVoice(assets.Wav(name), Gain(p ? p->voices_volume : 100));
}

void Game::Sound(const std::string& name) {
  Player* p = Cur();
  const davi::Sound* s = nullptr;
  if (name.rfind("__", 0) == 0) {
    auto it = embedded.find(name.substr(2));
    if (it != embedded.end()) s = &it->second;
  } else {
    s = assets.Wav(name);
  }
  app.audio.Play(s, Gain(p ? p->fx_volume : 50));
}

void Game::StartMusic() {
  Player* p = Cur();
  app.audio.PlayMusic(assets.Wav("menumsc"), Gain(p ? p->music_volume : 50));
}

void Game::Goto(davi::Screen* s) {
  if (app.Current() && app.Current() != s) prev_[s] = app.Current();
  app.Show(s);
}

void Game::Back() {
  auto it = prev_.find(app.Current());
  if (it != prev_.end()) app.Show(it->second);
}

}  // namespace a2
