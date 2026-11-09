// The dialogs (TfrmCoolDialog and its descendants).
#include <algorithm>
#include <memory>

#include "a2r4/screens.h"

namespace a2 {

// TfrmCoolDialog.GDIFormShow 0x488ff0: the dialog is as big as its picture
// (copy(ClassName, 5, 8), else the empty pop-up).
Dialog::Dialog(Game& g, const std::string& cls) : A2Screen(g, cls) {
  if (!form.background) form.background = g.Pic("popempty.tga");
  if (form.background) form.w = form.background->w, form.h = form.background->h;
  if (davi::Widget* b = form.Get("imgBackGround")) b->visible = false;
  form.SetVisible("btnHelp", false);  // no Agent help inside dialogs
  // btnOk1 without a definition of its own: the OK of a two-button dialog
  // (next to the 80x75 Cancel) or the single OK.
  if (davi::Widget* ok = form.Get("btnOk1"); ok && !ok->def) {
    ok->def = g.app.skin.Def(cls, form.Get("btnCancel") ? "Data.gdfBtnOkDlg" : "Data.gdfBtnOkSingleDlg");
    if (ok->def && ok->def->img[0]) ok->r.w = ok->def->w, ok->r.h = ok->def->h;
  }
}

void Dialog::Clicked(davi::Widget& w) {
  if (w.name == "btnOk1") return Ok();
  if (w.name == "btnCancel" || w.name == "btnClose" || w.name == "btnExit") return Close(2);
  Button(w);
}

bool Dialog::OnKey(SDL_Keycode k) {
  if (k == SDLK_ESCAPE) {
    Close(2);
    return true;
  }
  if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
    Ok();
    return true;
  }
  return false;
}

// ---- TfrmCloseDlg ---------------------------------------------------------

CloseDialog::CloseDialog(Game& g) : Dialog(g, "TfrmCloseDlg") {}

void CloseDialog::OnShow() {
  form.W("optMainScreen").checked = true;
  form.W("optQuit").checked = false;
}

// btnOk1Click 0x49e078: 1 = main menu, 3 = quit.
void CloseDialog::Ok() { Close(form.W("optMainScreen").checked ? 1 : 3); }

// TGameMenuScreen.btnCloseClick 0x49e9bc
void AskClose(Game& g) {
  g.app.ShowModal(g.Screen<CloseDialog>(), [&g](int r) {
    if (r == 3) {
      g.SavePlayers();
      g.app.Quit(0);
    } else if (r == 1) {
      g.Goto(g.Screen<MainScreen>());
    }
  });
}

// ---- TfrmStartInfo --------------------------------------------------------

StartInfoDialog::StartInfoDialog(Game& g, const std::string& cls) : Dialog(g, cls) {
  form.SetVisible("lblClickArea", false);
}

// GDIFormShow 0x4977a0, picture by device (v0E4 0x497818).
void StartInfoDialog::OnShow() {
  Player& p = *game.Cur();
  static const char* pics[4] = {"popstrt0", "popstrt2", "popstrt1", "popstrt3"};
  form.background = game.Pic(std::string(pics[std::clamp(p.device, 0, 3)]) + ".tga");
  if (form.background) form.w = form.background->w, form.h = form.background->h;
  form.W("chkStartInfo").checked = p.show_start_info;
}

void StartInfoDialog::Button(davi::Widget& w) {
  if (w.name == "lblClickArea") form.W("chkStartInfo").checked ^= true;
}

// btnOkClick 0x4977e0
void StartInfoDialog::Ok() {
  game.Cur()->show_start_info = form.W("chkStartInfo").checked;
  game.SavePlayers();
  Close(1);
}

StartInfoKeysDialog::StartInfoKeysDialog(Game& g) : StartInfoDialog(g, "TfrmStartInfoKeyBoard") {}

// GDIFormShow 0x4980d4: the keyboard picture with the player's keys.
void StartInfoKeysDialog::OnShow() {
  StartInfoDialog::OnShow();
  Player& p = *game.Cur();
  auto name = [&](int vk) {
    int n = game.menu.GetInt("CONTROLKEYS", "count", 0);
    for (int i = 0; i < n; i++) {
      auto v = davi::Split(game.menu.Get("CONTROLKEYS", std::to_string(i)), ',');
      if (v.size() >= 2 && std::atoi(v[0].c_str()) == vk) return v[1];
    }
    return std::to_string(vk);
  };
  form.SetCaption("lblSetGas", name(p.keys[0]));
  form.SetCaption("lblSetBrake", name(p.keys[1]));
  form.SetCaption("lblSetLeft", name(p.keys[2]));
  form.SetCaption("lblSetRight", name(p.keys[3]));
  form.SetCaption("lblSetTurbo", name(p.keys[4]));
  form.SetCaption("lblSetParkingBrake", name(p.keys[5]));
  form.SetCaption("lblSetHorn", name(p.keys[6]));
}

// ---- TfrmNewPlayer / TfrmHighScoreDlg -------------------------------------

NewPlayerDialog::NewPlayerDialog(Game& g) : Dialog(g, "TfrmNewPlayer") {}

void NewPlayerDialog::OnShow() {
  form.W("txtName").caption.clear();
  game.app.SetFocus(&form.W("txtName"));
}

void NewPlayerDialog::Ok() {
  name = davi::Trim(form.W("txtName").caption);
  if (name.empty()) return;
  Close(1);
}

void AskForNewPlayer(Game& g, std::function<void()> done) {
  auto* d = g.Screen<NewPlayerDialog>();
  g.app.ShowModal(d, [&g, d, done](int r) {
    if (r == 1 && !d->name.empty()) {
      g.NewPlayer(d->name);
      done();
    } else if (!g.Cur()) {
      AskForNewPlayer(g, done);  // a player is needed
    } else {
      done();
    }
  });
}

HighScoreDialog::HighScoreDialog(Game& g) : Dialog(g, "TfrmHighScoreDlg") {}

void HighScoreDialog::OnShow() {
  form.W("txtName").caption = game.Cur() ? game.Cur()->name : "";
  game.app.SetFocus(&form.W("txtName"));
}

// 0x49ed60: asked until a name is given.
void HighScoreDialog::Ok() {
  name = davi::Trim(form.W("txtName").caption);
  if (!name.empty()) Close(1);
}

// ---- TfrmPlayer / TfrmDeletePlayer ----------------------------------------

PlayerDialog::PlayerDialog(Game& g) : Dialog(g, "TfrmPlayer") {
  form.SetVisible("imgBack", false);
  static const char* suffix[] = {"up", "do", "fo", "hi", "di"};
  for (int n = 0; n <= 5; n++) {
    auto d = std::make_unique<davi::GraphicDef>();
    d->shared = true;
    for (int s = 0; s < davi::GraphicDef::kCount; s++) {
      std::string name = Indexed("pch", n) + "_" + suffix[s] + ".tga";
      if (g.assets.Has(name)) d->img[s] = g.Pic(name);
    }
    if (d->img[0]) d->w = d->img[0]->w, d->h = d->img[0]->h;
    d->caption = {10, 115, 125, 40};
    cards_.push_back(std::move(d));
  }
}

void PlayerDialog::OnShow() {
  chosen_ = game.current_player;
  Update();
}

// Five places: the players, then one "new player" place.
void PlayerDialog::Update() {
  slots_.clear();
  for (int i = 0; i < 5; i++) {
    davi::Widget& b = form.W("btn" + std::to_string(i));
    if (i < int(game.players.size())) {
      slots_.push_back(i);
      const Player& pl = game.players[i];
      int c = pl.current_char >= 0 && pl.current_char < int(game.tour_chars.size()) ? game.tour_chars[pl.current_char] : 5;
      b.def = cards_[std::clamp(c, 0, 5)].get();
      b.caption = pl.name;
      b.visible = true;
    } else if (i == int(game.players.size())) {
      slots_.push_back(-1);
      b.def = game.app.skin.Def("TfrmPlayer", "defEmpty");
      b.caption.clear();
      b.visible = true;
    } else {
      slots_.push_back(-2);
      b.visible = false;
    }
    b.font = {1, 18, SDL_Color{255, 255, 255, 255}};
    if (slots_.back() == chosen_) game.app.SetFocus(&b);
    if (b.def && b.def->img[0]) b.r.w = b.def->w, b.r.h = b.def->h;
  }
  if (chosen_ >= 0 && chosen_ < int(game.players.size()))
    form.SetCaption("lblTextBlock", Fmt(game.Str(9), game.players[chosen_].name));
  form.SetEnabled("btnDeletePlayer", chosen_ >= 0 && game.players.size() > 1);
}

void PlayerDialog::Button(davi::Widget& w) {
  if (w.name.rfind("btn", 0) == 0 && w.name.size() == 4 && isdigit(w.name[3])) {
    int s = slots_[w.name[3] - '0'];
    if (s >= 0) {
      chosen_ = s;
      Update();
    } else if (s == -1) {
      AskForNewPlayer(game, [this] {
        chosen_ = game.current_player;
        Update();
      });
    }
  } else if (w.name == "btnDeletePlayer" && chosen_ >= 0) {
    auto* d = game.Screen<DeletePlayerDialog>();
    d->Ask(game.players[chosen_].name);
    game.app.ShowModal(d, [this](int r) {
      if (r != 1 || chosen_ < 0) return;
      game.players.erase(game.players.begin() + chosen_);
      game.current_player = game.players.empty() ? -1 : 0;
      chosen_ = game.current_player;
      game.SavePlayers();
      Update();
    });
  }
}

void PlayerDialog::Ok() {
  if (chosen_ >= 0) game.current_player = chosen_;
  game.SavePlayers();
  game.StartMusic();
  Close(1);
}

DeletePlayerDialog::DeletePlayerDialog(Game& g) : Dialog(g, "TfrmDeletePlayer") {}

void DeletePlayerDialog::Ask(const std::string& name) { form.SetCaption("lblSure", Fmt(game.Str(4), name)); }

// ---- TfrmOptions ----------------------------------------------------------

OptionsDialog::OptionsDialog(Game& g) : Dialog(g, "TfrmOptions") {
  for (const char* n : {"imgPanel1", "imgPanel2", "imgControl", "imgVideo", "imgSound", "lblClickArea"})
    form.SetVisible(n, false);
}

// GDIFormShow 0x496a8c: a copy of the settings, kept on OK only.
void OptionsDialog::OnShow() {
  temp_ = *game.Cur();
  Show();
}

void OptionsDialog::Show() {
  const char* dev[4] = {"optKeyboard", "optJoyPad", "optJoystick", "optWheel"};
  for (int i = 0; i < 4; i++) form.W(dev[i]).checked = temp_.device == i;
  const char* res[3] = {"optVideoLow", "optVideoMedium", "optVideoHigh"};
  for (int i = 0; i < 3; i++) form.W(res[i]).checked = temp_.resolution == i + 1;
  form.W("chkVideo3d").checked = temp_.resolution >= 3;
  form.W("chkStartInfoOn").checked = temp_.show_start_info;
  form.W("sldrMusic").pos = temp_.music_volume;
  form.W("sldrSound").pos = temp_.fx_volume;
  form.W("sldrVoice").pos = temp_.voices_volume;
  // The device picture (opt_keyb / opt_gpad / opt_joys / opt_whee).
  static const char* pic[4] = {"opt_keyb.bmp", "opt_gpad.bmp", "opt_joys.bmp", "opt_whee.bmp"};
  form.SetPicture("imgControl", game.Pic(pic[std::clamp(temp_.device, 0, 3)]));
}

void OptionsDialog::Button(davi::Widget& w) {
  const char* dev[4] = {"optKeyboard", "optJoyPad", "optJoystick", "optWheel"};
  for (int i = 0; i < 4; i++)
    if (w.name == dev[i]) temp_.device = i;
  if (w.name == "optVideoLow") temp_.resolution = 1;
  if (w.name == "optVideoMedium") temp_.resolution = 2;
  if (w.name == "optVideoHigh") temp_.resolution = 3;
  if (w.name == "chkStartInfoOn") temp_.show_start_info = w.checked;
  if (w.name == "sldrMusic") {
    temp_.music_volume = w.pos;
    game.app.audio.SetMusicGain(w.pos / 100.0f);
  }
  if (w.name == "sldrSound") temp_.fx_volume = w.pos;
  if (w.name == "sldrVoice") temp_.voices_volume = w.pos;
}

void OptionsDialog::Ok() {
  *game.Cur() = temp_;
  game.SavePlayers();
  game.StartMusic();
  Close(1);
}

}  // namespace a2
