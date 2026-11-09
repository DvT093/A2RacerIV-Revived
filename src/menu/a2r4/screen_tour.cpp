// The Tour: character, current character, track and garage screens.
#include <algorithm>

#include "a2r4/screens.h"

namespace a2 {

namespace {

// A row of `count` pictures from lblAnc<x>00 with the step to lblAnc<x>01.
DotBar Row(davi::Form& f, SDL_Surface* pic, const std::string& a, const std::string& b, int count) {
  davi::Widget& w0 = f.W(a);
  davi::Widget& w1 = f.W(b);
  w0.visible = w1.visible = false;
  return {pic, w0.r.x, w0.r.y, w1.r.x - w0.r.x, count, 0};
}

void StatDots(davi::Form& f, Game& g, DotBar* dots) {
  davi::Widget& a00 = f.W("lblAncInd00");
  davi::Widget& a10 = f.W("lblAncInd10");
  DotBar r = Row(f, g.Pic("klrondje.tga"), "lblAncInd00", "lblAncInd01", 9);
  int dy = a10.r.y - a00.r.y;
  a10.visible = false;
  for (int i = 0; i < 4; i++) {
    dots[i] = r;
    dots[i].y += i * dy;
  }
}

// Speed, acceleration, road holding, brakes (rows of the dot block).
void SetStats(DotBar* dots, const Character* c) {
  dots[0].value = c ? c->speed : 0;
  dots[1].value = c ? c->accell : 0;
  dots[2].value = c ? c->handling : 0;
  dots[3].value = c ? c->brakes : 0;
}

}  // namespace

// ---- TfrmCharacter --------------------------------------------------------

CharacterScreen::CharacterScreen(Game& g) : A2Screen(g, "TfrmCharacter") {
  StatDots(form, g, dots_);
  // 0x49af5c: progress through the Tour, one big circle per track.
  progress_ = Row(form, g.Pic("butbigci.tga"), "lblAncTrackInd00", "lblAncTrackInd01", int(g.tour_tracks.size()));
  form.SetVisible("imgCar", false);
}

// FormShow 0x49ae00
void CharacterScreen::OnShow() {
  Player& p = *game.Cur();
  SetChar(std::max(0, p.current_char));
}

void CharacterScreen::OnHide() { game.app.CancelTimers(this); }

// SetCurrentCharIx 0x49ac68
void CharacterScreen::SetChar(int pos) {
  pos_ = pos;
  int c = game.tour_chars[pos];
  const Character* ch = game.Char(c);
  progress_.value = game.Cur()->Res(pos).tracktodrive;
  SetStats(dots_, ch);
  form.SetPicture("imgItemName", game.Pic(Indexed("chara_", c) + ".tga"));
  form.SetPicture("imgItem", game.Pic(Indexed("ch_", c) + ".tga"));
  form.SetCaption("lblTextBlock", ch ? ch->story : "");
}

void CharacterScreen::Clicked(davi::Widget& w) {
  int n = int(game.tour_chars.size());
  if (w.name == "btnNextChar") SetChar((pos_ + 1) % n);
  else if (w.name == "btnPrevChar") SetChar((pos_ + n - 1) % n);
}

void CharacterScreen::Paint(SDL_Surface* canvas, int ox, int oy) {
  for (auto& d : dots_) d.Paint(canvas, ox, oy);
  progress_.Paint(canvas, ox, oy);
}

// v0FC 0x49aac4: Hunny comments on the choice (CCS<char>_0x), then
// timSelectionTimer (2 s) goes on to the track.
void CharacterScreen::Next() {
  Player& p = *game.Cur();
  p.current_char = pos_;
  p.track_shown = p.Res(pos_).tracktodrive;
  game.SavePlayers();
  game.SpeakIdx("CCS", game.tour_chars[pos_], 2);
  form.SetEnabled("btnNext", false);
  game.app.After(2000, [this] {
    form.SetEnabled("btnNext", true);
    game.Goto(game.Screen<TrackScreen>());
  }, this);
}

void CharacterScreen::Previous() { game.Goto(game.Screen<DifficultyScreen>()); }

// ---- TfrmCurrentChar ------------------------------------------------------

CurrentCharScreen::CurrentCharScreen(Game& g) : A2Screen(g, "TfrmCurrentChar") {
  progress_ = Row(form, g.Pic("butbigci.tga"), "lblAncTrackInd00", "lblAncTrackInd01", int(g.tour_tracks.size()));
}

// FormShow 0x49b4fc
void CurrentCharScreen::OnShow() {
  CharResult& r = *game.TourRes();
  int c = game.TourChar();
  form.SetCaption("lblPoints", std::to_string(r.money));
  form.SetPicture("imgItemName", game.Pic(Indexed("chara_", c) + ".tga"));
  form.SetPicture("imgItem", game.Pic(Indexed("ch_", c) + ".tga"));
  progress_.value = r.tracktodrive;
  game.Voice("aud3");
}

void CurrentCharScreen::Clicked(davi::Widget& w) {
  if (w.name == "btnChooseChar") game.Goto(game.Screen<CharacterScreen>());  // 0x49b4e8
}

void CurrentCharScreen::Paint(SDL_Surface* canvas, int ox, int oy) { progress_.Paint(canvas, ox, oy); }

void CurrentCharScreen::Next() {
  Player& p = *game.Cur();
  p.track_shown = game.TourRes()->tracktodrive;
  game.Goto(game.Screen<TrackScreen>());
}

void CurrentCharScreen::Previous() { game.Goto(game.Screen<DifficultyScreen>()); }

// ---- TfrmTrack ------------------------------------------------------------

TrackScreen::TrackScreen(Game& g) : A2Screen(g, "TfrmTrack") {
  progress_ = Row(form, g.Pic("butbigci.tga"), "lblAncTrackInd00", "lblAncTrackInd01", int(g.tour_tracks.size()));
  form.SetVisible("lblAncFocus", false);
  form.SetPicture("imgFocus", g.Pic("indfocus.tga"));
}

// FormShow 0x49a168
void TrackScreen::OnShow() {
  form.SetPicture("imgItem", game.Pic("hunny.tga"));
  form.SetPicture("imgShadow", game.Pic("shadow.tga"));
  CharResult& r = *game.TourRes();
  progress_.value = r.tracktodrive;
  // FormShow: the track to drive (0x499d68 with CharResult.tracktodrive).
  SetTrack(std::min(r.tracktodrive, int(game.tour_tracks.size()) - 1));
}

void TrackScreen::OnHide() { game.app.CancelTimers(this); }

// SetCurrentTrackIndex 0x499d68 and the texts (0x499f80).
void TrackScreen::SetTrack(int pos) {
  Player& p = *game.Cur();
  CharResult& r = *game.TourRes();
  p.track_shown = pos;
  const Track& t = game.tracks[game.TourTrack()];
  form.SetPicture("imgTrackName", game.Pic(Indexed("trname", t.index) + ".tga"));
  // Hunny announces a race; for a chase D.T. Harry's shadow stands there.
  form.SetVisible("imgItem", !t.IsChase());
  form.SetVisible("imgShadow", t.IsChase());
  davi::Widget& focus = form.W("imgFocus");
  focus.r.x = progress_.x + progress_.step * pos;
  const Game::Record& rec = game.records[t.index];
  form.SetCaption("lblTime", rec.time_ms > 0 ? game.FormatTime(rec.time_ms) : "");
  form.SetCaption("lblRecName", rec.name);
  form.SetCaption("lblType", t.type);
  form.SetCaption("lblTextBlock", t.story);
  if (t.IsChase()) {
    form.SetCaption("lblQualify", game.Str(8));  // "Ontsnappen"
  } else {
    int q = p.difficulty < int(t.qualify.size()) ? t.qualify[p.difficulty] : 0;
    form.SetCaption("lblQualify", Fmt(game.Str(14), game.Ordinal(q + 1)));  // "%s Plaats"
  }
  form.SetEnabled("btnNextTrack", pos < std::min(r.tracktodrive, int(game.tour_tracks.size()) - 1));
  form.SetEnabled("btnPrevTrack", pos > 0);
  cycle_ = 5;
  flip_ = false;
  game.app.CancelTimers(this);
  Cycle();
  // tmrSoundTimer 0x49a5c0: after 1.5 s "CTI08_<track>" (chase) or "CTI07_<track>".
  game.app.After(1500, [this, t] { game.Voice(davi::Lower(Indexed(t.IsChase() ? "cti08_" : "cti07_", t.index))); }, this);
}

// 0x49a328: slide show ttcNN_00..05.
void TrackScreen::Cycle() {
  cycle_ = cycle_ == 5 ? 0 : cycle_ + 1;
  std::string show = flip_ ? "imgTrack2" : "imgTrack1", hide = flip_ ? "imgTrack1" : "imgTrack2";
  form.SetPicture(show, game.Pic(Indexed("ttc", game.TourTrack()) + "_" + Indexed("", cycle_) + ".tga"));
  form.SetVisible(show, true);
  form.SetVisible(hide, false);
  flip_ = !flip_;
  game.app.After(2000, [this] { Cycle(); }, this);
}

void TrackScreen::Clicked(davi::Widget& w) {
  int pos = game.Cur()->track_shown;
  if (w.name == "btnNextTrack") SetTrack(pos + 1);
  else if (w.name == "btnPrevTrack") SetTrack(pos - 1);
}

void TrackScreen::Paint(SDL_Surface* canvas, int ox, int oy) { progress_.Paint(canvas, ox, oy); }

// v0FC 0x499c7c: the shown track is the one to drive.
void TrackScreen::Next() {
  game.SavePlayers();
  game.Goto(game.Screen<GarageScreen>());
}

void TrackScreen::Previous() { game.Goto(game.Screen<CurrentCharScreen>()); }

// ---- TfrmGarage -----------------------------------------------------------

GarageScreen::GarageScreen(Game& g) : A2Screen(g, "TfrmGarage") {
  // 0x498df8: five rows of nine (speed, acceleration, road holding, brakes,
  // turbo): "blind_up" for an empty place, "klrondje" for a point and
  // "redin_up" for what the tune-up shown adds.
  dot_ = g.Pic("klrondje.tga");
  blind_ = g.Pic("blind_up.tga");
  red_ = g.Pic("redin_up.tga");
  davi::Widget& a00 = form.W("lblAncInd00");
  davi::Widget& a01 = form.W("lblAncInd01");
  davi::Widget& a10 = form.W("lblAncInd10");
  for (int i = 0; i < 5; i++) bars_.push_back({a00.r.x, a00.r.y + i * (a10.r.y - a00.r.y), a01.r.x - a00.r.x, 0, 0});
  a00.visible = a01.visible = a10.visible = false;
}

// FormShow 0x4990a0
void GarageScreen::OnShow() {
  SetItem(0);
  // 0x499538: after a Tour race Hunny says something (good or bad).
  if (game.last.started && game.mode == 2) {
    if (game.rrflags & 2) game.Speak("CGR07", 3);
    else game.Speak("CBR07", 6);
    game.last.started = false;
  }
}

// 0x4993d4
void GarageScreen::SetItem(int i) {
  item_ = std::clamp(i, 0, std::min(4, int(game.tuneups.size()) - 1));
  form.SetPicture("imgItemName", game.Pic(Indexed("tune_", item_) + ".tga"));
  form.SetPicture("imgItem", game.Pic(Indexed("tuneup", item_) + ".tga"));
  Update();
}

// 0x499464, 0x4996a0, 0x499600
void GarageScreen::Update() {
  CharResult& r = *game.TourRes();
  const Character* ch = game.Char(game.TourChar());
  const TuneUp& tu = game.tuneups[item_];
  form.SetCaption("lblPoints", std::to_string(r.money));
  form.SetCaption("lblTextBlock", tu.text);
  form.SetCaption("lblPrice", Fmt(game.Str(13), std::to_string(tu.price)));
  int base[4] = {ch ? ch->speed : 0, ch ? ch->accell : 0, ch ? ch->handling : 0, ch ? ch->brakes : 0};
  for (int k = 0; k < 4 && k < int(game.tuneups.size()); k++) {
    if (!r.tuneup[k]) continue;
    const TuneUp& t = game.tuneups[k];
    base[0] += t.dspeed, base[1] += t.dacc, base[2] += t.dhandl, base[3] += t.dbrakes;
  }
  int extra[4] = {};
  if (item_ < 4 && !r.tuneup[item_]) extra[0] = tu.dspeed, extra[1] = tu.dacc, extra[2] = tu.dhandl, extra[3] = tu.dbrakes;
  for (int k = 0; k < 4; k++) bars_[k].base = base[k], bars_[k].extra = extra[k];
  bars_[4].base = r.turbo * 9 / 100;
  bars_[4].extra = 0;
  form.SetEnabled("btnBuyTuneUp", item_ == 4 ? r.turbo < 100 : !r.tuneup[item_]);
}

// btnBuyTuneUpClick 0x4991f4
void GarageScreen::Buy() {
  CharResult& r = *game.TourRes();
  const TuneUp& tu = game.tuneups[item_];
  if (r.money < tu.price) {
    static MessageDialog dlg(game, "TfrmNoMoney");
    game.app.ShowModal(&dlg);
    return;
  }
  game.Sound("peep");
  r.money -= tu.price;
  if (item_ == 4) r.turbo = std::min(100, r.turbo + game.stat.GetInt("GAMEVALUES", "TURBOOILAMOUNT", 10));
  else r.tuneup[item_] = true;
  game.SavePlayers();
  Update();
}

void GarageScreen::Clicked(davi::Widget& w) {
  if (w.name == "btnNextTU") SetItem(item_ == 4 ? 0 : item_ + 1);
  else if (w.name == "btnPrevTU") SetItem(item_ == 0 ? 4 : item_ - 1);
  else if (w.name == "btnBuyTuneUp") Buy();
}

void GarageScreen::Paint(SDL_Surface* canvas, int ox, int oy) {
  for (auto& b : bars_) {
    for (int i = 0; i < 9; i++) {
      SDL_Surface* s = i < b.base ? dot_ : i < b.base + b.extra ? red_ : blind_;
      if (!s) continue;
      SDL_Rect d{ox + b.x + i * b.step, oy + b.y, s->w, s->h};
      SDL_BlitSurface(s, nullptr, canvas, &d);
    }
  }
}

// v0FC 0x4991ac
void GarageScreen::Next() { StartRace(game); }

// v100 0x499524
void GarageScreen::Previous() { game.Goto(game.Screen<TrackScreen>()); }

}  // namespace a2
