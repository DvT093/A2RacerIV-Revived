// Own render size for the race: the RED engine renders at any 4:3 size with
// a HUD that scales along, it only needs another size than its mode table
// has. Same engine as Vakantie Racer (its gfx_resolution.cpp, own copy
// here). See re/findings.md ("Resolutie").

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "x86/runtime.h"

namespace a2r4 {

// Set by main.cpp: >0 = the size, <0 = off (the mode table).
int g_render_w = -1, g_render_h = -1;

// The largest 4:3 size (pixels) on screen: the whole screen when the race
// runs fullscreen, else the usable area minus the title bar. Started from
// the menu: the largest 4:3 size inside the menu window (A2R4_WINDOW).
// Before recomp::RunProcess SDL video is still off, so on and off again
// (without signal handlers, the way SwARc starts it).
void ScreenSize(int* w, int* h) {
  *w = 1024;
  *h = 768;
  if (const char* e = std::getenv("A2R4_WINDOW")) {
    int x, y, pw, ph, fs = 0;
    float d = 1;
    if (std::sscanf(e, "%d,%d,%d,%d,%d,%f", &x, &y, &pw, &ph, &fs, &d) >= 4 && pw > 0 && ph > 0) {
      if (!(d > 0)) d = 1;
      int ww = int(pw * d), hh = int(ph * d);
      hh = std::min(hh, ww * 3 / 4) & ~1;
      *h = hh;
      *w = (hh * 4 / 3) & ~1;
      return;
    }
  }
  SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
  if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) return;
  SDL_DisplayID d = SDL_GetPrimaryDisplay();
  const SDL_DisplayMode* m = SDL_GetDesktopDisplayMode(d);
  float density = m && m->pixel_density > 0 ? m->pixel_density : 1.0f;
  const char* fs = std::getenv("A2R4_FULLSCREEN");
  const char* bg = std::getenv("A2R4_BACKGROUND");
  bool window = fs ? !std::strcmp(fs, "window") : (bg && *bg && *bg != '0');
  float pw = m ? float(m->w) : 1024, ph = m ? float(m->h) : 768;  // points
  SDL_Rect r;
  if (window && SDL_GetDisplayUsableBounds(d, &r)) {
    pw = float(r.w);
    ph = float(r.h) - 28;  // macOS title bar
  }
  int hh = int(ph * density), ww = int(pw * density);
  hh = std::min(hh, ww * 3 / 4) & ~1;
  *h = hh;
  *w = (hh * 4 / 3) & ~1;
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

}  // namespace a2r4

namespace recomp::gen {

void f_004C4330_lifted(::recomp::Ctx& c);

// 0x4C4330 (cdecl kind, value): kind 0 copies mode `value` of the mode
// table 0x5B37B0 (21 x {w, h, bpp, lores, 0}) into the active size
// 0x5B3798 (width) and 0x5B379C (height). Raser.ini RESOLUTION 3 = index 9
// (640x480x16). For the 16-bit 3D modes 9..13 we then set our own size.
void f_004C4330(::recomp::Ctx& c) {
  const uint32_t kind = Rd32(c.esp + 4), index = Rd32(c.esp + 8);
  f_004C4330_lifted(c);
  if (kind != 0 || index < 9 || index > 13 || a2r4::g_render_w < 0) return;
  int w = a2r4::g_render_w, h = a2r4::g_render_h;
  Wr32(0x5B3798, uint32_t(w));
  Wr32(0x5B379C, uint32_t(h));
  std::fprintf(stderr, "a2r4: rendermaat %dx%d (modus %u)\n", w, h, index);
}

}  // namespace recomp::gen
