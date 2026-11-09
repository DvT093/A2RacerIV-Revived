// a2r4: A2 Racer IV (Davilex, 2000), natively.
//
//   a2r4 [--game-dir DIR] [--cd-dir DIR] [-- GAME ARGS...]
//
// DIR is the game's data folder from the user's own CD (the folder with
// spel.dat, rcs.IMG, game.ini). Defaults to $A2R4_GAME_DIR. The disc
// itself (volume label A2RACER) is the folder above DIR; override with
// --cd-dir. Set A2R4_TRACE=1 to log every HLE call.

#include <sys/stat.h>

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <SDL3/SDL.h>

#include "runtime/log.h"
#include "win32/process.h"

namespace a2r4 {
extern int g_render_w, g_render_h;  // game/gfx_resolution.cpp
void ScreenSize(int* w, int* h);
}  // namespace a2r4

int main(int argc, char** argv) {
  std::string game_dir, cd_dir, game_args;
  if (const char* e = std::getenv("A2R4_GAME_DIR")) game_dir = e;
  for (int i = 1; i < argc; i++) {
    if (!std::strcmp(argv[i], "--game-dir") && i + 1 < argc) {
      game_dir = argv[++i];
    } else if (!std::strcmp(argv[i], "--cd-dir") && i + 1 < argc) {
      cd_dir = argv[++i];
    } else if (!std::strcmp(argv[i], "--")) {
      for (i++; i < argc; i++) {
        if (!game_args.empty()) game_args += ' ';
        game_args += argv[i];
      }
    } else {
      std::fprintf(stderr, "usage: %s [--game-dir DIR] [--cd-dir DIR] [-- GAME ARGS...]\n", argv[0]);
      return 2;
    }
  }
  if (game_dir.empty()) {
    std::fprintf(stderr, "a2r4: pass --game-dir or set A2R4_GAME_DIR\n");
    return 2;
  }
  while (game_dir.size() > 1 && game_dir.back() == '/') game_dir.pop_back();
  if (const char* t = std::getenv("A2R4_TRACE")) recomp::g_trace = *t && *t != '0';

  std::string exe = game_dir + "/spel.dat";
  struct stat st;
  if (stat(exe.c_str(), &st) != 0) {
    std::fprintf(stderr, "a2r4: %s not found\n", exe.c_str());
    return 2;
  }

  recomp::win32::GameConfig game;
  game.id = "a2r4";
  game.guest_dir = "C:\\A2 Racer IV\\Data";
  game.exe_name = "spel.dat";
  if (cd_dir.empty()) cd_dir = game_dir.substr(0, game_dir.find_last_of('/'));
  game.cd_drives = {{'D', cd_dir, "A2RACER"}};
  // Same engine as Vakantie Racer: no frame limiter, simulation per frame.
  // 60 fps as on the hardware of its day; A2R4_MAX_FPS=0 uncaps (SwARc).
  game.max_fps = 60;

  // Image upgrades, as in Vakantie Racer: render at the screen's own 4:3
  // size, MSAA with alpha-to-coverage, mipmaps with trilinear and 16x
  // anisotropic filtering, HD textures. F9 switches all of it between
  // original and upgraded in the race, F10 only anti-aliasing, F11 only
  // filtering. A2R4_RESOLUTION=original|WxH overrides the size.
  a2r4::ScreenSize(&a2r4::g_render_w, &a2r4::g_render_h);
  if (const char* r = std::getenv("A2R4_RESOLUTION")) {
    int w = 0, h = 0;
    if (!std::strcmp(r, "original")) a2r4::g_render_w = a2r4::g_render_h = -1;
    else if (std::sscanf(r, "%dx%d", &w, &h) == 2 && w >= 320 && h >= 240) a2r4::g_render_w = w, a2r4::g_render_h = h;
  }
  game.gfx.msaa = 8;
  game.gfx.alpha_to_coverage = true;
  game.gfx.mipmaps = true;
  game.gfx.anisotropy = 16;
  game.gfx_enhanced = true;
  // Turbo on Cmd: macOS keeps Ctrl+arrow for itself (Spaces), so the race
  // sees the Command keys as Ctrl and the real Ctrl keys not at all.
  game.key_aliases = {{SDL_SCANCODE_LGUI, SDL_SCANCODE_LCTRL},
                      {SDL_SCANCODE_RGUI, SDL_SCANCODE_RCTRL},
                      {SDL_SCANCODE_LCTRL, 0},
                      {SDL_SCANCODE_RCTRL, 0}};
  game.gfx_key_all = SDL_SCANCODE_F9;
  game.gfx_key_aa = SDL_SCANCODE_F10;
  game.gfx_key_filter = SDL_SCANCODE_F11;
  game.gfx_label_original = "Origineel";
  game.gfx_label_enhanced = "Upgraded";
  // HD textures (tools/make_texture_pack.py, from your own CD): SwARc
  // knows each texture by its pixels and draws the 4x version while
  // Upgraded is on (F9). A2R4_TEX_PACK=dir overrides the place.
  game.gfx.replace_textures = true;
  {
    char self[PATH_MAX];
    if (realpath(argv[0], self)) {
      std::string root = self;  // <project>/out/build/release/a2r4
      for (int up = 0; up < 4; up++) root = root.substr(0, root.find_last_of('/'));
      if (stat((root + "/build/texture_pack").c_str(), &st) == 0) game.texture_pack = root + "/build/texture_pack";
    }
  }
  return recomp::RunProcess(game, exe, game_dir, game_args);
}
