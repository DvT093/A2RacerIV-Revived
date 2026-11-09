// Mouse.cpp of the original (D:\Work\dxdraw): reading the mouse while the
// game shuts down.

#include "x86/runtime.h"

namespace recomp::gen {

void f_004CECE0_lifted(::recomp::Ctx& c);

// 0x4CECE0 handle_mouse: only the message pump 0x4CD660 calls it, when no
// message is waiting. Closing the window ends in the shutdown 0x4C4040,
// which turns the mouse off (0x4CE490, flag 0x74B2F4 = 0), posts WM_CLOSE
// to the window that is already gone and pumps again: no message, so
// handle_mouse, which then stops the game with the error box "Mouse.cpp(772)
// (handle_mouse) Mouse is not initialised" and ExitProcess(1). With the
// mouse off there is nothing to read: return, and the shutdown goes on.
void f_004CECE0(::recomp::Ctx& c) {
  if (Rd32(0x74B2F4) == 0) {
    c.eax = 0;
    c.esp += 4;  // ret
    return;
  }
  f_004CECE0_lifted(c);
}

}  // namespace recomp::gen
