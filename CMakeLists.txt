cmake_minimum_required(VERSION 3.25)
project(a2r4 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_LINK_DEPENDS_NO_SHARED ON)

# SwARc (lifter, x87 model, runtime, Win32 HLE): A2's own worktree
# ../SwARc-a2r4 (branch a2r4), never the Vakantie Racer one.
set(SWARC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../SwARc-a2r4" CACHE PATH "SwARc source tree")

if(EXISTS "${SWARC_DIR}/CMakeLists.txt")
  enable_testing()
  add_subdirectory("${SWARC_DIR}" swarc)

  # spel.dat lifted to C++ in generated/ (./lift.sh, see CLAUDE.md).
  recomp_add_lifted(a2r4_lifted "${CMAKE_CURRENT_SOURCE_DIR}/generated")
  if(TARGET a2r4_lifted)
    # src/game: hand-written replacements of lifted functions
    # (lifter.manifest "replace"), one file per original source file.
    file(GLOB A2R4_GAME_SOURCES CONFIGURE_DEPENDS src/game/*.cpp)
    add_executable(a2r4 src/main.cpp ${A2R4_GAME_SOURCES} $<TARGET_OBJECTS:a2r4_lifted>)
    target_include_directories(a2r4 PRIVATE src)
    target_link_libraries(a2r4 PRIVATE recomp_win32 recomp_d3d7 recomp_dinput8 recomp_dsound)
    set_target_properties(a2r4 PROPERTIES ENABLE_EXPORTS ON)
  endif()
endif()

# Native menu (replaces MenuData/Menu.exe), see src/menu/.
add_subdirectory(src/menu)
