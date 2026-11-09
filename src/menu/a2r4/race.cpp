// Starting a race and what follows it, as Menu.exe did it:
// TSaveGame.WriteGeneral/Control/QR/VRStuffToIni (0x48bcf0, 0x48b350,
// 0x48b5bc, 0x48b7c8) before the race, TfrmMain 0x4a5d70 after it
// (0x4a54dc/0x4a58a4 Quick Race, 0x4a5624/0x4a59cc Tour).
#include <algorithm>

#include "a2r4/screens.h"

namespace a2 {

namespace {

enum { kChase = 1, kQualify = 2, kWinRace = 4, kWinTrack = 8, kBestLap = 32 };

int RaceTrack(Game& g) { return g.mode == 1 ? g.QrTrack() : g.TourTrack(); }
int RaceChar(Game& g) { return g.mode == 1 ? g.QrChar() : g.TourChar(); }

void WriteRaceKeys(Game& g) {
  Player& p = *g.Cur();
  red::RaceLauncher& L = g.launcher;
  const Track& t = g.tracks[RaceTrack(g)];
  const Character* ch = g.Char(RaceChar(g));
  // WriteGeneralStuffToIni 0x48bcf0
  L.SetInt("ETAPPE", "ETAPPE", t.indexingame);
  L.SetInt("OPTIONS", "NRLAPS", t.nrlaps);
  L.SetInt("OPTIONS", "OPPONENTS", t.IsChase() ? 0 : 1);
  L.SetInt("ETAPPE", "BRITISH", t.british ? 1 : 0);
  L.SetInt("LEAGUE", "LEAGUE", p.difficulty);
  L.SetInt("PLAYER", "DAMAGE", 0);
  if (t.IsChase())
    L.SetInt("PLAYER", "CHECKPOINT_SPEED",
             g.stat.GetInt("CHECKPOINTTIME", "Difficulty" + std::to_string(p.difficulty), 71));
  L.SetInt("GFX OPTIONS", "gamma", p.gamma);
  L.SetInt("GFX OPTIONS", "alpha_blending", p.alpha);
  L.SetInt("OPTIONS", "waitvbl", p.waitvbl);
  L.SetInt("OPTIONS", "filter", p.bilinear ? 2 : 0);
  // WriteControlStuffToIni 0x48b350: 1 keyboard, 2 joypad, 3 wheel/joystick.
  L.SetInt("INPUT", "DEVICE", std::min(3, p.device + 1));
  static const char* keys[7] = {"UP", "DOWN", "LEFT", "RIGHT", "EXTRA1", "EXTRA2", "EXTRA3"};
  for (int k = 0; k < 7; k++) L.SetInt("INPUT", keys[k], p.keys[k]);
  L.SetInt("INPUT", "SENSITIVITY", p.device == 3 ? p.wheel_sensitivity : p.joy_sensitivity);
  int car = ch ? ch->indexingame : 0;
  if (g.mode == 1) {  // WriteQRStuffToIni 0x48b5bc
    L.SetInt("PLAYER", "MONEY", 0);
    L.SetInt("PLAYER", "CAR", car);
    L.SetInt("PLAYER", "TURBO_AMOUNT", 100);
    for (const char* k : {"MOTOR", "BANDEN", "ABS", "VERSNELLINGSBAK"}) L.SetInt("PLAYER", k, 1);
  } else {  // WriteVRStuffToIni 0x48b7c8
    CharResult& r = *g.TourRes();
    L.SetInt("PLAYER", "MONEY", r.money);
    L.SetInt("PLAYER", "CAR", car);
    const char* tk[4] = {"MOTOR", "BANDEN", "ABS", "VERSNELLINGSBAK"};
    for (int k = 0; k < 4; k++) L.SetInt("PLAYER", tk[k], r.tuneup[k]);
    L.SetInt("PLAYER", "TURBO_AMOUNT", r.turbo);
  }
  L.SetInt("PLAYER", "POSITION", 4);
}

// 0x48003c: the computer racers of [RACERS] and the player, by time.
void Rank(Game& g) {
  const red::RaceResult& r = g.last;
  struct E { int ch, time; bool me; };
  std::vector<E> v;
  for (size_t i = 0; i < r.racers.size() && i < 4; i++)
    v.push_back({g.CharByIndexInGame(r.racers[i].car), r.racers[i].time_ms, false});
  v.push_back({RaceChar(g), r.tracktime, true});
  std::stable_sort(v.begin(), v.end(), [](const E& a, const E& b) { return a.time < b.time; });
  g.ranking.clear();
  for (size_t i = 0; i < v.size(); i++) {
    g.ranking.push_back({v[i].ch, v[i].time});
    if (v[i].me) g.playerpos = int(i);
  }
}

// 0x49ed60: a lap faster than the record of the track replaces it; the
// name is asked for right away (TfrmHighScoreDlg).
void BestLap(Game& g) {
  davi::IniFile gi = g.launcher.LoadGameIni();
  Game::Record& rec = g.records[g.race_track];
  for (int lap = 1; lap <= 5; lap++) {
    int t = gi.GetInt("PLAYER", "LAP_" + std::to_string(lap) + "_TIME");
    if (t > 0 && (rec.time_ms <= 0 || t < rec.time_ms)) {
      g.rrflags |= kBestLap;
      rec.time_ms = t;
    }
  }
}

// 0x48dd00: the next Tour track, but only when the race was the one to drive.
void Advance(Game& g) {
  CharResult& r = *g.TourRes();
  if (g.Cur()->track_shown == r.tracktodrive && r.tracktodrive < int(g.tour_tracks.size())) r.tracktodrive++;
}

void TakeResults(Game& g) {
  const red::RaceResult& res = g.last;
  g.rrflags = 0;
  g.playerpos = 0;
  g.ranking.clear();
  g.race_track = RaceTrack(g);
  const Track& t = g.tracks[g.race_track];
  if (t.IsChase()) g.rrflags |= kChase;
  if (g.mode == 1) {  // 0x4a54dc
    if (res.finished && !t.IsChase()) Rank(g);
    if (res.finished) BestLap(g);
    return;
  }
  // Tour, 0x4a5624
  if (!res.finished) return;
  Player& p = *g.Cur();
  CharResult& r = *g.TourRes();
  for (bool& b : r.tuneup) b = false;  // 0x48dc38: tune-ups last one race
  davi::IniFile gi = g.launcher.LoadGameIni();
  r.money = std::max(0, gi.GetInt("PLAYER", "MONEY", r.money));
  r.turbo = std::clamp(res.turbo_amount, 0, 100);
  if (t.IsChase()) {
    BestLap(g);
    // 0x49eef0: escaped = qualified; after the last track the Tour is won.
    g.rrflags |= kQualify;
    bool last = p.track_shown == int(g.tour_tracks.size()) - 1;
    Advance(g);
    if (last) {
      g.rrflags |= kWinRace;
      r.won = true;
      if (!p.supercop) p.supercop = p.supercop_new = true;
    }
  } else {
    Rank(g);
    BestLap(g);
    if (g.playerpos == 0) {  // 0x49f010: winner's bonus
      Advance(g);
      g.rrflags |= kWinTrack;
      r.money += g.stat.GetInt("GAMEVALUES", "FirstPlaceBonus", 2500);
    } else {
      int q = p.difficulty < int(t.qualify.size()) ? t.qualify[p.difficulty] : 0;
      if (g.playerpos <= q) {  // 0x49f100
        g.rrflags |= kQualify;
        Advance(g);
      }
    }
  }
  g.SavePlayers();
}

// 0x4a58a4 / 0x4a59cc
void AfterRace(Game& g) {
  const red::RaceResult& res = g.last;
  bool chase = g.rrflags & kChase;
  davi::Screen* next = nullptr;
  if (g.mode == 1) {
    next = res.finished && !chase ? (davi::Screen*)g.Screen<ResultsScreen>() : g.Screen<QuickRaceScreen>();
  } else if (!res.finished) {
    davi::IniFile gi = g.launcher.LoadGameIni();
    bool caught = gi.GetInt("PLAYER", "Out_Of_Time") || gi.GetInt("PLAYER", "Total_Loss");
    next = chase && caught ? (davi::Screen*)g.Screen<LoseChaseScreen>() : g.Screen<TrackScreen>();
  } else if (!chase) {
    next = g.Screen<ResultsScreen>();
  } else if (!(g.rrflags & kWinRace)) {
    next = g.Screen<WinChaseScreen>();
  } else {
    next = g.Screen<WinFinalScreen>();
  }
  auto go = [&g, next] { g.Goto(next); };
  if (g.rrflags & kBestLap) {
    auto* d = g.Screen<HighScoreDialog>();
    g.app.ShowModal(d, [&g, d, go](int) {
      g.records[g.race_track].name = d->name;
      g.SaveRecords();
      go();
    });
    return;
  }
  go();
}

void RunRace(Game& g) {
  WriteRaceKeys(g);
  g.SavePlayers();
  g.last = g.launcher.Run(g.app);
  g.StartMusic();
  if (!g.last.started) {  // no race binary yet: the race itself is not ported
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "A2 Racer IV",
                             "De race zelf is nog niet geport naar de Mac. Het menu heeft game.ini wel klaargezet.",
                             nullptr);
    return;
  }
  TakeResults(g);
  AfterRace(g);
}

}  // namespace

// QuickRace/Garage v0FC: the start info first when it is on (0x49812c).
void StartRace(Game& g) {
  Player& p = *g.Cur();
  if (!p.show_start_info) return RunRace(g);
  Dialog* d = p.device == 0 ? (Dialog*)g.Screen<StartInfoKeysDialog>() : g.Screen<StartInfoDialog>();
  g.app.ShowModal(d, [&g](int res) {
    if (res == 1) RunRace(g);
  });
}

}  // namespace a2
