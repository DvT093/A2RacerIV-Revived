// After a race: results, won / qualified / lost, the chases and the end of
// the Tour.
#include <algorithm>

#include "a2r4/screens.h"

namespace a2 {

namespace {

// [charN] of the winner of the last race (first of the ranking).
int Winner(Game& g) { return g.ranking.empty() ? g.TourChar() : g.ranking[0].ch; }

}  // namespace

// ---- TfrmResults ----------------------------------------------------------

ResultsScreen::ResultsScreen(Game& g) : A2Screen(g, "TfrmResults") {
  bg_tour_ = g.Pic("results.bmp");
  bg_quick_ = g.Pic("quickres.bmp");  // v0E4 0x4a0b5c
  for (const char* n : {"lblTopImage", "lbl2ndImage", "lblFocus", "lblTime"}) form.SetVisible(n, false);
}

// FormShow 0x4a0624
void ResultsScreen::OnShow() {
  form.background = game.mode == 1 ? bg_quick_ : bg_tour_;
  if (davi::Widget* b = form.Get("imgBackGround")) b->picture = form.background;
  game.Voice("aud2");
  form.SetPicture("imgFocus", game.Pic("focus.tga"));
  form.SetPicture("imgItem", game.Pic(Indexed("win_ch", Winner(game)) + ".tga"));
  davi::Widget& top = form.W("lblTopImage");
  davi::Widget& second = form.W("lbl2ndImage");
  davi::Widget& focus = form.W("imgFocus");
  focus.r.x = form.W("lblFocus").r.x;
  focus.r.y = form.W("lblFocus").r.y + (second.r.y - top.r.y) * game.playerpos;
}

// One row per racer: res_chNN and the time (the others: " + " the gap).
void ResultsScreen::Paint(SDL_Surface* canvas, int ox, int oy) {
  davi::Widget& top = form.W("lblTopImage");
  davi::Widget& second = form.W("lbl2ndImage");
  davi::Widget& time = form.W("lblTime");
  int dy = second.r.y - top.r.y;
  for (size_t i = 0; i < game.ranking.size(); i++) {
    if (SDL_Surface* s = game.Pic(Indexed("res_ch", game.ranking[i].ch) + ".tga")) {
      SDL_Rect d{ox + top.r.x, oy + top.r.y + int(i) * dy, s->w, s->h};
      SDL_BlitSurface(s, nullptr, canvas, &d);
    }
    int t = game.ranking[i].time_ms;
    std::string txt = i == 0 ? game.FormatTime(t) : " + " + game.FormatTime(t - game.ranking[0].time_ms);
    davi::TextStyle st{game.app.skin.GetFont(time.font.font), time.font.px, time.font.color};
    SDL_Rect r{ox + time.r.x, oy + time.r.y + int(i) * dy, 200, 40};
    davi::DrawText(canvas, r, txt, st);
  }
}

// v0FC 0x4a0a20
void ResultsScreen::Next() {
  if (game.mode == 1) game.Goto(game.Screen<QuickRaceScreen>());
  else if (game.rrflags & 8) game.Goto(game.Screen<WinNormalScreen>());
  else if (game.rrflags & 2) game.Goto(game.Screen<QualifyScreen>());
  else game.Goto(game.Screen<LoseNormalScreen>());
}

// ---- TfrmWinNormal --------------------------------------------------------

WinNormalScreen::WinNormalScreen(Game& g) : A2Screen(g, "TfrmWinNormal") {}

// FormShow 0x4a0218, texts 0x4a0304
void WinNormalScreen::OnShow() {
  const Track& t = game.tracks[game.race_track];
  int c = game.TourChar();
  form.SetCaption("lblTextBlock", t.wintext);
  form.SetCaption("lblBonus", Fmt(game.Str(10), std::to_string(game.stat.GetInt("GAMEVALUES", "FirstPlaceBonus", 2500))));
  form.SetPicture("imgTrackName", game.Pic(Indexed("trname", t.index) + ".tga"));
  form.SetPicture("imgItem", game.Pic(Indexed("win_ch", c) + ".tga"));
  game.SpeakIdx("CDW", c, 2);
  // timSampleTimer 0x4a03b0: after 4.5 s "CDW07_<track>".
  game.app.After(4500, [this, t] { game.Voice(davi::Lower(Indexed("cdw07_", t.index))); }, this);
}

void WinNormalScreen::OnHide() { game.app.CancelTimers(this); }
void WinNormalScreen::Next() { game.Goto(game.Screen<TrackScreen>()); }

// ---- TfrmQualify ----------------------------------------------------------

QualifyScreen::QualifyScreen(Game& g) : A2Screen(g, "TfrmQualify") {}

// FormShow 0x49fe68
void QualifyScreen::OnShow() {
  const Track& t = game.tracks[game.race_track];
  int w = Winner(game);
  form.SetPicture("imgTrackName", game.Pic(Indexed("trname", t.index) + ".tga"));
  form.SetCaption("lblTextBlock", t.qualifytext);
  form.SetPicture("imgItem", game.Pic(Indexed("win_ch", w) + ".tga"));
  form.SetPicture("imgWinnerName", game.Pic(Indexed("chara_", w) + ".tga"));
  game.SpeakIdx("CDW", w, 2);
}

void QualifyScreen::OnHide() { game.app.CancelTimers(this); }
void QualifyScreen::Next() { game.Goto(game.Screen<TrackScreen>()); }

// ---- TfrmLoseNormal -------------------------------------------------------

LoseNormalScreen::LoseNormalScreen(Game& g) : A2Screen(g, "TfrmLoseNormal") {}

// FormShow 0x49f50c
void LoseNormalScreen::OnShow() {
  const Track& t = game.tracks[game.race_track];
  int c = game.TourChar();
  form.SetPicture("imgTrackName", game.Pic(Indexed("trname", t.index) + ".tga"));
  form.SetCaption("lblPoints", std::to_string(game.TourRes()->money));
  form.SetCaption("lblTextBlock", t.losetext);
  form.SetPicture("imgItem", game.Pic(Indexed("los_ch", c) + ".tga"));
  game.SpeakIdx("CDL", c, 2);
}

void LoseNormalScreen::OnHide() { game.app.CancelTimers(this); }
void LoseNormalScreen::Next() { game.Goto(game.Screen<TrackScreen>()); }  // v0FC 0x49f4f8

// ---- TfrmWinChase / TfrmLoseChase -----------------------------------------

WinChaseScreen::WinChaseScreen(Game& g) : A2Screen(g, "TfrmWinChase") {}

// FormShow 0x4a0dc8: D.T. Harry is angry, the club got away.
void WinChaseScreen::OnShow() {
  const Track& t = game.tracks[game.race_track];
  form.SetCaption("lblTextBlock", t.wintext);
  form.SetPicture("imgTrackName", game.Pic(Indexed("trname", t.index) + ".tga"));
  form.SetPicture("imgItem", game.Pic("copangry.tga"));
  game.Voice(davi::Lower(Indexed("cdw05_", t.index)));
}

void WinChaseScreen::Next() { game.Goto(game.Screen<TrackIntroScreen>()); }  // v0FC 0x4a0db4

LoseChaseScreen::LoseChaseScreen(Game& g) : A2Screen(g, "TfrmLoseChase") {}

// FormShow 0x4a10a8: caught.
void LoseChaseScreen::OnShow() {
  const Track& t = game.tracks[game.race_track];
  form.SetPicture("imgTrackName", game.Pic(Indexed("trname", t.index) + ".tga"));
  form.SetPicture("imgItem", game.Pic("cophappy.tga"));
  game.Voice(davi::Lower(Indexed("cdl05_", t.index)));
  form.SetCaption("lblTextBlock", t.losetext);
}

void LoseChaseScreen::Next() { game.Goto(game.Screen<TrackScreen>()); }  // v0FC 0x4a1094

// ---- TfrmTrackIntro -------------------------------------------------------

TrackIntroScreen::TrackIntroScreen(Game& g) : A2Screen(g, "TfrmTrackIntro") {}

// FormShow 0x49fa34: "NextTr" sound, timClose 3.5 s, then the track screen.
void TrackIntroScreen::OnShow() {
  game.Sound("nexttr");
  game.app.After(3500, [this] { Done(); }, this);
}

void TrackIntroScreen::OnHide() { game.app.CancelTimers(this); }
void TrackIntroScreen::Clicked(davi::Widget&) { Done(); }
bool TrackIntroScreen::OnKey(SDL_Keycode) {
  Done();
  return true;
}
void TrackIntroScreen::Done() {
  game.app.CancelTimers(this);
  game.Goto(game.Screen<TrackScreen>());
}

// ---- TfrmWinFinal ---------------------------------------------------------

WinFinalScreen::WinFinalScreen(Game& g) : A2Screen(g, "TfrmWinFinal") {
  davi::Widget& tl = form.W("lblAncTopLeft");
  davi::Widget& br = form.W("lblAncBottomRight");
  movie_rect_ = {tl.r.x, tl.r.y, br.r.x - tl.r.x, br.r.y - tl.r.y};
  tl.visible = br.visible = false;
  form.SetVisible("pnlMovie", false);
}

// FormShow 0x4a3f44: the winner's text and movie (MenuData\CHnn.avi),
// timSample1/2: "CCW08_<char>" then "CCW08_05".
void WinFinalScreen::OnShow() {
  int c = game.TourChar();
  const Character* ch = game.Char(c);
  form.SetCaption("lblTextBlock", ch ? ch->winracetext : "");
  std::string avi = davi::FindNoCase(game.launcher.install().game_dir, "MenuData/" + Indexed("CH", c) + ".avi");
  Player* p = game.Cur();
  if (!avi.empty()) movie_.Open(avi, &game.app.audio, (p ? p->fx_volume : 50) / 100.0f);
  game.Voice(davi::Lower(Indexed("ccw08_", c)));
  game.app.After(6000, [this] { game.Voice("ccw08_05"); }, this);
}

void WinFinalScreen::OnHide() {
  game.app.CancelTimers(this);
  movie_.Close();
}

void WinFinalScreen::Tick(double) {
  if (movie_.IsOpen() && !movie_.Update()) movie_.Restart();
}

void WinFinalScreen::Paint(SDL_Surface* canvas, int ox, int oy) {
  if (SDL_Surface* f = movie_.Frame()) {
    SDL_Rect d{ox + movie_rect_.x, oy + movie_rect_.y, movie_rect_.w, movie_rect_.h};
    SDL_BlitSurfaceScaled(f, nullptr, canvas, &d, SDL_SCALEMODE_LINEAR);
  }
}

// v0FC 0x4a42e8: with D.T. Harry just unlocked to Quick Race (he can be
// picked there now), otherwise to a new character.
void WinFinalScreen::Next() {
  Player& p = *game.Cur();
  if (p.supercop_new) {
    game.mode = 1;
    game.Goto(game.Screen<QuickRaceScreen>());
  } else {
    p.current_char = -1;
    game.Goto(game.Screen<CharacterScreen>());
  }
}

}  // namespace a2
