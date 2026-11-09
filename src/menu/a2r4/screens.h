// The A2 Racer IV screens, one class per original Delphi form (TfrmXxx in
// Menu.exe). Addresses in the comments are Menu.exe's (re/menu_*.c).
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "a2r4/game.h"
#include "engine/video.h"

namespace a2 {

void Start(Game& g, bool intro);
// TfrmCloseDlg: back to the main menu (1) or quit (3).
void AskClose(Game& g);
// Asks a name until one is given and makes the player (TfrmNewPlayer).
void AskForNewPlayer(Game& g, std::function<void()> done);
// Start-info popup (when on), then the race, then the screen that follows
// (TfrmMain.FormActivate 0x4a4fe8 -> 0x4a5d70).
void StartRace(Game& g);
// "prefix" + %02d, the way Menu.exe names its numbered pictures (0x489e40).
std::string Indexed(const std::string& prefix, int n);

// TDotBar (0x491590): a row of the same picture, the first `value` shown.
struct DotBar {
  SDL_Surface* pic = nullptr;
  int x = 0, y = 0, step = 0, count = 9, value = 0;
  void Paint(SDL_Surface* canvas, int ox, int oy) const;
};

class MainScreen : public A2Screen {  // TfrmMainScreen (mainscre.bmp)
 public:
  explicit MainScreen(Game& g);
  void OnShow() override;
  void Clicked(davi::Widget& w) override;
  void Next() override;
  void Previous() override {}
};

class DifficultyScreen : public A2Screen {  // TfrmDifficulty (difficul / quickdif)
 public:
  explicit DifficultyScreen(Game& g);
  void OnShow() override;
  void Clicked(davi::Widget& w) override;
  void Next() override;
  void Previous() override;

 private:
  SDL_Surface* bg_tour_ = nullptr;
  SDL_Surface* bg_quick_ = nullptr;
};

class QuickRaceScreen : public A2Screen {  // TfrmQuickRace (quickrac)
 public:
  explicit QuickRaceScreen(Game& g);
  void OnShow() override;
  void OnHide() override;
  void Clicked(davi::Widget& w) override;
  void Next() override;
  void Previous() override;
  void Paint(SDL_Surface* canvas, int ox, int oy) override;

 private:
  void SetTrack(int pos);
  void SetChar(int pos);
  bool TrackAvailable(int pos) const;
  bool CharAvailable(int pos) const;
  void UpdateNext();
  void Cycle();
  DotBar dots_[4];
  int cycle_ = 5;
  bool flip_ = false;
};

class CharacterScreen : public A2Screen {  // TfrmCharacter (characte)
 public:
  explicit CharacterScreen(Game& g);
  void OnShow() override;
  void OnHide() override;
  void Clicked(davi::Widget& w) override;
  void Next() override;
  void Previous() override;
  void Paint(SDL_Surface* canvas, int ox, int oy) override;

 private:
  void SetChar(int pos);
  DotBar dots_[4], progress_;
  int pos_ = 0;
};

class CurrentCharScreen : public A2Screen {  // TfrmCurrentChar (currentc)
 public:
  explicit CurrentCharScreen(Game& g);
  void OnShow() override;
  void Clicked(davi::Widget& w) override;
  void Next() override;
  void Previous() override;
  void Paint(SDL_Surface* canvas, int ox, int oy) override;

 private:
  DotBar progress_;
};

class TrackScreen : public A2Screen {  // TfrmTrack (track)
 public:
  explicit TrackScreen(Game& g);
  void OnShow() override;
  void OnHide() override;
  void Clicked(davi::Widget& w) override;
  void Next() override;
  void Previous() override;
  void Paint(SDL_Surface* canvas, int ox, int oy) override;

 private:
  void SetTrack(int pos);
  void Cycle();
  DotBar progress_;
  int cycle_ = 5;
  bool flip_ = false;
};

class GarageScreen : public A2Screen {  // TfrmGarage (garage)
 public:
  explicit GarageScreen(Game& g);
  void OnShow() override;
  void Clicked(davi::Widget& w) override;
  void Next() override;
  void Previous() override;
  void Paint(SDL_Surface* canvas, int ox, int oy) override;

 private:
  void SetItem(int i);
  void Buy();
  void Update();
  int item_ = 0;  // [TUNEUPS] 0-3, 4 = turbo oil
  struct Bar {
    int x, y, step, base, extra;
  };
  std::vector<Bar> bars_;
  SDL_Surface *dot_ = nullptr, *blind_ = nullptr, *red_ = nullptr;
};

// After the race.
class ResultsScreen : public A2Screen {  // TfrmResults (results / QUICKRES)
 public:
  explicit ResultsScreen(Game& g);
  void OnShow() override;
  void Next() override;
  void Paint(SDL_Surface* canvas, int ox, int oy) override;

 private:
  SDL_Surface* bg_tour_ = nullptr;
  SDL_Surface* bg_quick_ = nullptr;
};

class WinNormalScreen : public A2Screen {  // TfrmWinNormal (winnorma)
 public:
  explicit WinNormalScreen(Game& g);
  void OnShow() override;
  void OnHide() override;
  void Next() override;
};

class QualifyScreen : public A2Screen {  // TfrmQualify (qualify)
 public:
  explicit QualifyScreen(Game& g);
  void OnShow() override;
  void OnHide() override;
  void Next() override;
};

class LoseNormalScreen : public A2Screen {  // TfrmLoseNormal (losenorm)
 public:
  explicit LoseNormalScreen(Game& g);
  void OnShow() override;
  void OnHide() override;
  void Next() override;
};

class WinChaseScreen : public A2Screen {  // TfrmWinChase (winchase)
 public:
  explicit WinChaseScreen(Game& g);
  void OnShow() override;
  void Next() override;
};

class LoseChaseScreen : public A2Screen {  // TfrmLoseChase (losechas)
 public:
  explicit LoseChaseScreen(Game& g);
  void OnShow() override;
  void Next() override;
};

class WinFinalScreen : public A2Screen {  // TfrmWinFinal (winfinal)
 public:
  explicit WinFinalScreen(Game& g);
  void OnShow() override;
  void OnHide() override;
  void Next() override;
  void Tick(double dt) override;
  void Paint(SDL_Surface* canvas, int ox, int oy) override;

 private:
  davi::Video movie_;
  SDL_Rect movie_rect_{};
};

class TrackIntroScreen : public A2Screen {  // TfrmTrackIntro (trackint)
 public:
  explicit TrackIntroScreen(Game& g);
  void OnShow() override;
  void OnHide() override;
  void Clicked(davi::Widget& w) override;
  bool OnKey(SDL_Keycode k) override;

 private:
  void Done();
};

class IntroScreen : public A2Screen {  // TfrmIntro: menu.ini INTROAVI (intro1.mpg)
 public:
  explicit IntroScreen(Game& g);
  bool Play(const std::string& path);
  void Clicked(davi::Widget& w) override;
  bool OnKey(SDL_Keycode k) override;
  void Tick(double dt) override;
  void Paint(SDL_Surface* canvas, int ox, int oy) override;

 private:
  void Done();
  davi::Video movie_;
  bool done_ = false;
};

// Dialogs (TfrmCoolDialog descendants): btnOk1 closes with 1, btnCancel /
// btnClose with 2 (Delphi mrOk / mrCancel).
class Dialog : public A2Screen {
 public:
  Dialog(Game& g, const std::string& cls);
  void Clicked(davi::Widget& w) final;
  bool OnKey(SDL_Keycode k) override;
  virtual void Button(davi::Widget& w) { (void)w; }
  virtual void Ok() { Close(1); }
};

class CloseDialog : public Dialog {  // TfrmCloseDlg (closedlg)
 public:
  explicit CloseDialog(Game& g);
  void OnShow() override;
  void Ok() override;
};

class StartInfoDialog : public Dialog {  // TfrmStartInfo (popstrt0-3)
 public:
  StartInfoDialog(Game& g, const std::string& cls = "TfrmStartInfo");
  void OnShow() override;
  void Button(davi::Widget& w) override;
  void Ok() override;
};

class StartInfoKeysDialog : public StartInfoDialog {  // TfrmStartInfoKeyBoard
 public:
  explicit StartInfoKeysDialog(Game& g);
  void OnShow() override;
};

class MessageDialog : public Dialog {  // TfrmNoMoney, TfrmUnlock, TfrmInternetDlg
 public:
  MessageDialog(Game& g, const std::string& cls) : Dialog(g, cls) {}
};

class NewPlayerDialog : public Dialog {  // TfrmNewPlayer (newplaye)
 public:
  explicit NewPlayerDialog(Game& g);
  void OnShow() override;
  void Ok() override;
  std::string name;
};

class HighScoreDialog : public Dialog {  // TfrmHighScoreDlg (highscor)
 public:
  explicit HighScoreDialog(Game& g);
  void OnShow() override;
  void Ok() override;
  std::string name;
};

class PlayerDialog : public Dialog {  // TfrmPlayer (player)
 public:
  explicit PlayerDialog(Game& g);
  void OnShow() override;
  void Button(davi::Widget& w) override;
  void Ok() override;

 private:
  void Update();
  std::vector<int> slots_;  // player index per btn0..4
  int chosen_ = -1;
  // defCharNN (0x4946e8): the card of a player, "pchNN" of the Tour
  // character, caption rect (10,115)-(135,155).
  std::vector<std::unique_ptr<davi::GraphicDef>> cards_;
};

class DeletePlayerDialog : public Dialog {  // TfrmDeletePlayer
 public:
  explicit DeletePlayerDialog(Game& g);
  void Ask(const std::string& name);
};

class OptionsDialog : public Dialog {  // TfrmOptions (options)
 public:
  explicit OptionsDialog(Game& g);
  void OnShow() override;
  void Button(davi::Widget& w) override;
  void Ok() override;

 private:
  void Show();
  Player temp_;
};

// Hunny's help (MS Agent in the original): her picture with the screen's
// text from Agent.ini.
class HelpPopup : public A2Screen {
 public:
  explicit HelpPopup(Game& g);
  void Open(const std::string& form_cls);
  void Clicked(davi::Widget& w) override;
  bool OnKey(SDL_Keycode k) override;
  void Paint(SDL_Surface* canvas, int ox, int oy) override;

 private:
  std::string text_;
};

}  // namespace a2
