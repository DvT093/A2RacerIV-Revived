// TGameMenuScreen / TBaseFormOneWay / TBaseFormTwoWay: what every A2 screen shares.
#include "a2r4/game.h"
#include "a2r4/screens.h"

namespace a2 {

A2Screen::A2Screen(Game& g, const std::string& cls) : davi::Screen(g.app, cls), game(g) {
  // btnHelp: Hunny (MS Agent) explains the screen; there is no Agent on the Mac,
  // so the native menu shows her picture with the text (see Help()).
  if (davi::Widget* h = form.Get("btnHelp")) h->caption.clear();
  if (davi::Widget* c = form.Get("btnClose")) c->caption.clear();
}

void A2Screen::Update(double dt) { Tick(dt); }

void A2Screen::PaintOver(SDL_Surface* canvas, int ox, int oy) { Paint(canvas, ox, oy); }

void A2Screen::OnClick(davi::Widget& w) {
  if (w.name == "btnNext" && !IsPopup()) return Next();
  if (w.name == "btnPrevious" && !IsPopup()) return Previous();
  if (w.name == "btnHelp") return Help();
  if (w.name == "btnClose" && !IsPopup()) return AskClose(game);
  Clicked(w);
}

bool A2Screen::OnKey(SDL_Keycode k) {
  if (k == SDLK_ESCAPE && !IsPopup()) {
    if (form.Get("btnPrevious") && form.W("btnPrevious").visible) Previous();
    else AskClose(game);
    return true;
  }
  return false;
}

// TBaseFormTwoWay.btnPreviousClick 0x4913c0
void A2Screen::Previous() { game.Back(); }

}  // namespace a2
