// TfrmQuickRace: pick a track and a character, then race.
#include <algorithm>

#include "a2r4/screens.h"

namespace a2 {

namespace {

// Dot rows of the four stats: anchored on lblAncInd00, one row down per
// lblAncInd10, one dot to the right per lblAncInd01 (0x49c868).
void PlaceDots(davi::Form& f, Game& g, DotBar* dots, int n) {
  davi::Widget& a00 = f.W("lblAncInd00");
  davi::Widget& a01 = f.W("lblAncInd01");
  davi::Widget& a10 = f.W("lblAncInd10");
  int step = a01.r.x - a00.r.x, dy = a10.r.y - a00.r.y;
  for (int i = 0; i < n; i++) dots[i] = {g.Pic("klrondje.tga"), a00.r.x, a00.r.y + i * dy, step, 9, 0};
  a00.visible = a01.visible = a10.visible = false;
}

}  // namespace

QuickRaceScreen::QuickRaceScreen(Game& g) : A2Screen(g, "TfrmQuickRace") {
  PlaceDots(form, g, dots_, 4);
  form.SetVisible("lblLocked", false);
  form.SetVisible("lblCharLocked", false);
}

// A track can be raced when its level is on disk (the download track is
// not) and, for a chase, when a Tour character has got past it (0x49c78c,
// 0x48c644).
bool QuickRaceScreen::TrackAvailable(int pos) const {
  if (pos < 0 || pos >= int(game.qr_tracks.size())) return false;
  const Track& t = game.tracks[game.qr_tracks[pos]];
  // In the install or the overlay (Davilex's bonus track, etappe9).
  std::string rmp = "etappe" + std::to_string(t.indexingame) + ".rmp";
  const red::Install& in = game.launcher.install();
  if (davi::FindNoCase(in.user_dir + "/install", rmp).empty() && davi::FindNoCase(in.game_dir, rmp).empty())
    return false;
  if (!t.IsChase()) return true;
  auto it = std::find(game.tour_tracks.begin(), game.tour_tracks.end(), t.index);
  if (it == game.tour_tracks.end()) return true;
  int tpos = int(it - game.tour_tracks.begin());
  const Player* p = const_cast<Game&>(game).Cur();
  if (!p) return false;
  for (auto& r : p->results)
    if (r.tracktodrive > tpos || r.won) return true;
  return false;
}

// 0x49ca08: the download character when Davilex's car update is installed
// (Game::qr_download_available); D.T. Harry (LOCKEDCHARIX) after winning
// the Tour once.
bool QuickRaceScreen::CharAvailable(int pos) const {
  if (pos == game.qr_download_char) return game.qr_download_available;
  if (pos == game.qr_locked_char) {
    const Player* p = const_cast<Game&>(game).Cur();
    return p && p->supercop;
  }
  return true;
}

// FormShow 0x49bdec
void QuickRaceScreen::OnShow() {
  Player& p = *game.Cur();
  SetTrack(std::clamp(p.qr_track, 0, std::max(0, int(game.qr_tracks.size()) - 1)));
  SetChar(std::clamp(p.qr_char, 0, std::max(0, int(game.qr_chars.size()) - 1)));
  game.mode = 1;
  if (p.supercop_new) {  // TfrmUnlock: D.T. Harry is now available
    p.supercop_new = false;
    game.SavePlayers();
    static MessageDialog dlg(game, "TfrmUnlock");
    game.Sound("unlock");
    game.app.ShowModal(&dlg);
  }
}

void QuickRaceScreen::OnHide() { game.app.CancelTimers(this); }

// SetCurrentTrackIndex 0x49c198
void QuickRaceScreen::SetTrack(int pos) {
  Player& p = *game.Cur();
  p.qr_track = pos;
  const Track& t = game.tracks[game.qr_tracks[pos]];
  form.SetPicture("imgTrackName", game.Pic(Indexed("trname", t.index) + ".tga"));
  const Game::Record& rec = game.records[t.index];
  form.SetCaption("lblTime", rec.time_ms > 0 ? game.FormatTime(rec.time_ms) : "");
  form.SetCaption("lblRecName", rec.name);
  form.SetCaption("lblType", t.type);
  bool ok = TrackAvailable(pos);
  form.SetVisible("lblLocked", !ok);
  form.SetCaption("lblLocked", game.Str(t.IsChase() ? 7 : 2));
  // 0x49cb94: restart the slide show of the track (qtcNN_00..05).
  cycle_ = 5;
  flip_ = false;
  form.SetVisible("imgTrack2", false);
  form.SetVisible("imgTrack1", true);
  game.app.CancelTimers(this);
  Cycle();
  UpdateNext();
}

// timCyclerTimer 0x49ca50: the next picture, on the other of two images.
void QuickRaceScreen::Cycle() {
  cycle_ = cycle_ == 5 ? 0 : cycle_ + 1;
  int t = game.qr_tracks[game.Cur()->qr_track];
  std::string show = flip_ ? "imgTrack2" : "imgTrack1", hide = flip_ ? "imgTrack1" : "imgTrack2";
  form.SetPicture(show, game.Pic(Indexed("qtc", t) + "_" + Indexed("", cycle_) + ".tga"));
  form.SetVisible(show, true);
  form.SetVisible(hide, false);
  flip_ = !flip_;
  game.app.After(2000, [this] { Cycle(); }, this);
}

// SetCurrentCharIx 0x49c4d8
void QuickRaceScreen::SetChar(int pos) {
  Player& p = *game.Cur();
  p.qr_char = pos;
  int c = game.qr_chars[pos];
  const Character* ch = game.Char(c);
  bool ok = CharAvailable(pos);
  form.SetVisible("lblCharLocked", !ok);
  form.SetCaption("lblCharLocked", ch ? ch->lockedtext : "");
  dots_[0].value = ch ? ch->speed : 0;
  dots_[1].value = ch ? ch->accell : 0;
  dots_[2].value = ch ? ch->handling : 0;
  dots_[3].value = ch ? ch->brakes : 0;
  form.SetPicture("imgCharName", game.Pic(Indexed("chara_", c) + ".tga"));
  form.SetPicture("imgItem", game.Pic(Indexed("qr_ch", c) + ".tga"));
  UpdateNext();
}

// 0x49c3c8
void QuickRaceScreen::UpdateNext() {
  Player& p = *game.Cur();
  form.SetEnabled("btnNext", TrackAvailable(p.qr_track) && CharAvailable(p.qr_char));
}

void QuickRaceScreen::Clicked(davi::Widget& w) {
  Player& p = *game.Cur();
  int nt = int(game.qr_tracks.size()), nc = int(game.qr_chars.size());
  if (w.name == "btnNextTrack") SetTrack((p.qr_track + 1) % nt);
  else if (w.name == "btnPrevTrack") SetTrack((p.qr_track + nt - 1) % nt);
  else if (w.name == "btnNextChar") SetChar((p.qr_char + 1) % nc);
  else if (w.name == "btnPrevChar") SetChar((p.qr_char + nc - 1) % nc);
}

void QuickRaceScreen::Paint(SDL_Surface* canvas, int ox, int oy) {
  for (auto& d : dots_) d.Paint(canvas, ox, oy);
}

// v0FC 0x49bf18
void QuickRaceScreen::Next() {
  game.SavePlayers();
  StartRace(game);
}

// v100 0x49bf04
void QuickRaceScreen::Previous() { game.Goto(game.Screen<DifficultyScreen>()); }

}  // namespace a2
