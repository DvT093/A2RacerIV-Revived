// Main menu, difficulty, intro movie and the start of the menu.
#include <algorithm>
#include <cstdio>

#include "a2r4/screens.h"

namespace a2 {

std::string Indexed(const std::string& prefix, int n) {
  char b[16];
  std::snprintf(b, sizeof b, "%02d", n);
  return prefix + b;
}

void DotBar::Paint(SDL_Surface* canvas, int ox, int oy) const {
  if (!pic) return;
  for (int i = 0; i < count && i < value; i++) {
    SDL_Rect d{ox + x + i * step, oy + y, pic->w, pic->h};
    SDL_BlitSurface(pic, nullptr, canvas, &d);
  }
}

// ---- TfrmMainScreen -------------------------------------------------------

MainScreen::MainScreen(Game& g) : A2Screen(g, "TfrmMainScreen") {
  form.SetVisible("imgCar", false);  // easter-egg hot spot
}

// FormShow 0x49d894: Quick Race (1) or Tour (2) selected, its picture in imgItem.
void MainScreen::OnShow() {
  bool qr = game.mode == 1;
  form.W("optQuickRace").checked = qr;
  form.W("optTour").checked = !qr;
  form.SetPicture("imgItem", game.Pic(qr ? "quick.tga" : "tour.tga"));
  game.StartMusic();
}

void MainScreen::Clicked(davi::Widget& w) {
  if (w.name == "optQuickRace" || w.name == "optTour") {
    game.mode = w.name == "optQuickRace" ? 1 : 2;
    form.SetPicture("imgItem", game.Pic(game.mode == 1 ? "quick.tga" : "tour.tga"));
  } else if (w.name == "btnPlayer") {
    game.app.ShowModal(game.Screen<PlayerDialog>());
  } else if (w.name == "btnOptions") {
    game.app.ShowModal(game.Screen<OptionsDialog>(), [this](int) { game.StartMusic(); });
  } else if (w.name == "btnInternet") {
    // TfrmInternetDlg asked to visit the Davilex site, which is long gone.
    static MessageDialog dlg(game, "TfrmInternetDlg");
    game.app.ShowModal(&dlg);
  }
}

// v0FC 0x49da44: remember the choice, go to the difficulty screen.
void MainScreen::Next() {
  game.mode = form.W("optTour").checked ? 2 : 1;
  game.Goto(game.Screen<DifficultyScreen>());
}

// ---- TfrmDifficulty -------------------------------------------------------

DifficultyScreen::DifficultyScreen(Game& g) : A2Screen(g, "TfrmDifficulty") {
  // v0E4 0x49d2b4: "quickdif" for Quick Race, "difficul" for the Tour.
  bg_tour_ = g.Pic("difficul.bmp");
  bg_quick_ = g.Pic("quickdif.bmp");
}

// FormShow 0x49cf00
void DifficultyScreen::OnShow() {
  bool qr = game.mode == 1;
  form.background = qr ? bg_quick_ : bg_tour_;
  if (davi::Widget* b = form.Get("imgBackGround")) b->picture = form.background;
  if (davi::Widget* h = form.Get("imgItemName")) h->picture = game.Pic(qr ? "headquic.tga" : "headtour.tga");
  int d = game.Cur() ? game.Cur()->difficulty : 1;
  form.W("optEasy").checked = d == 0;
  form.W("optMedium").checked = d == 1;
  form.W("optHard").checked = d == 2;
  form.SetPicture("imgItem", game.Pic(Indexed("diffic", d) + ".tga"));
}

void DifficultyScreen::Clicked(davi::Widget& w) {
  int d = w.name == "optEasy" ? 0 : w.name == "optMedium" ? 1 : w.name == "optHard" ? 2 : -1;
  if (d >= 0) form.SetPicture("imgItem", game.Pic(Indexed("diffic", d) + ".tga"));
}

// v0FC 0x49d124
void DifficultyScreen::Next() {
  Player& p = *game.Cur();
  p.difficulty = form.W("optEasy").checked ? 0 : form.W("optMedium").checked ? 1 : 2;
  game.SavePlayers();
  if (game.mode == 1) game.Goto(game.Screen<QuickRaceScreen>());
  else if (p.current_char >= 0) game.Goto(game.Screen<CurrentCharScreen>());
  else game.Goto(game.Screen<CharacterScreen>());
}

// v100 0x49d2a0
void DifficultyScreen::Previous() { game.Goto(game.Screen<MainScreen>()); }

// ---- TfrmIntro ------------------------------------------------------------

IntroScreen::IntroScreen(Game& g) : A2Screen(g, "TfrmIntro") {}

bool IntroScreen::Play(const std::string& path) {
  done_ = false;
  Player* p = game.Cur();
  return movie_.Open(path, &game.app.audio, (p ? p->music_volume : 50) / 100.0f);
}

void IntroScreen::Clicked(davi::Widget&) { Done(); }
bool IntroScreen::OnKey(SDL_Keycode) {  // FormKeyDown 0x4a4a78: any key skips
  Done();
  return true;
}

void IntroScreen::Tick(double) {
  if (!done_ && !movie_.Update()) Done();
}

void IntroScreen::Paint(SDL_Surface* canvas, int ox, int oy) {
  SDL_Surface* f = movie_.Frame();
  if (!f) return;
  // Scaled to fit 800x600 (the original played it full screen).
  float sc = std::min(800.0f / f->w, 600.0f / f->h);
  int w = int(f->w * sc), h = int(f->h * sc);
  SDL_Rect d{ox + (800 - w) / 2, oy + (600 - h) / 2, w, h};
  SDL_BlitSurfaceScaled(f, nullptr, canvas, &d, SDL_SCALEMODE_LINEAR);
}

void IntroScreen::Done() {
  if (done_) return;
  done_ = true;
  movie_.Close();
  game.app.After(0, [this] { Start(game, false); }, this);
}

// ---- Start ----------------------------------------------------------------

// TfrmMain.FormActivate 0x4a4fe8 / 0x4a50d0 / 0x4a525c: the intro, then a
// player (asked for when there is none), then the main menu.
void Start(Game& g, bool intro) {
  if (intro) {
    std::string rel = g.menu.Get("GENERAL", "INTROAVI", "DATA\\INTRO1.MPG");
    if (davi::Lower(rel).rfind("data\\", 0) == 0) rel = rel.substr(5);
    std::string path = davi::FindNoCase(g.launcher.install().game_dir, rel);
    auto* s = g.Screen<IntroScreen>();
    if (!path.empty() && s->Play(path)) {
      g.Goto(s);
      return;
    }
  }
  if (g.mode == 0) g.mode = 2;
  g.Goto(g.Screen<MainScreen>());
  if (!g.Cur()) AskForNewPlayer(g, [] {});
}

// ---- Help (MS Agent) ------------------------------------------------------

void A2Screen::Help() {
  auto* h = game.Screen<HelpPopup>();
  h->Open(form.cls);
  game.app.ShowModal(h);
}

HelpPopup::HelpPopup(Game& g) : A2Screen(g, "TBalloonForm") {
  form.w = 800;
  form.h = 600;
  for (auto& w : form.widgets) w->visible = false;
}

// Agent.ini [<form>] Caption and [<form>BalloonItems]: Hunny's questions.
// The answers were topics of Menu.hlp (WinHelp) and are not shown.
void HelpPopup::Open(const std::string& cls) {
  text_ = game.agent.Get(cls, "Caption");
  for (auto& k : game.agent.Keys(cls + "BalloonItems")) text_ += "\n- " + k;
  game.Speak("cmh07", 1);  // btnHelpClick 0x49e678
}

void HelpPopup::Clicked(davi::Widget&) { Close(0); }
bool HelpPopup::OnKey(SDL_Keycode) {
  Close(0);
  return true;
}

void HelpPopup::Paint(SDL_Surface* canvas, int ox, int oy) {
  SDL_Surface* h = game.Pic("hunny.tga");
  int hx = 800 - (h ? h->w : 0) - 10, hy = 600 - (h ? h->h : 0) - 40;
  if (h) {
    SDL_Rect d{ox + hx, oy + hy, h->w, h->h};
    SDL_BlitSurface(h, nullptr, canvas, &d);
  }
  SDL_Rect box{ox + hx - 330, oy + hy + 20, 320, 220};
  SDL_FillSurfaceRect(canvas, &box, SDL_MapSurfaceRGB(canvas, 0xff, 0xff, 0xe1));
  davi::TextStyle st{game.app.skin.GetFont(0), 15, SDL_Color{0, 0, 0, 255}, davi::HAlign::kLeft,
                     davi::VAlign::kTop, true};
  SDL_Rect tr{box.x + 10, box.y + 8, box.w - 20, box.h - 16};
  davi::DrawText(canvas, tr, text_, st);
}

}  // namespace a2
