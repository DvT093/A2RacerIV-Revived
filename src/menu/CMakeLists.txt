# a2r4-menu: native replacement for A2 Racer IV's Delphi menu (MenuData/Menu.exe).
#   engine/   menu engine (own copy, started from Vakantie Racer's; A2 adds the
#             MenuRcs archive, TGA pictures and the TGDI components)
#   launcher/ RED-engine race handshake (game.ini + running the race)
#   a2r4/     A2 Racer IV screens and flow
find_package(SDL3 REQUIRED CONFIG)
find_package(Freetype REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MENU_FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libavutil libswscale libswresample)

add_library(a2r4_menu_engine STATIC
  engine/dfm.cpp engine/ini.cpp engine/text.cpp engine/assets.cpp
  engine/form.cpp engine/app.cpp engine/video.cpp
  launcher/race_launcher.cpp)
target_include_directories(a2r4_menu_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(a2r4_menu_engine PUBLIC SDL3::SDL3 Freetype::Freetype PkgConfig::MENU_FFMPEG)

file(GLOB A2R4_MENU_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/a2r4/*.cpp)
add_executable(a2r4-menu ${A2R4_MENU_SOURCES})
target_link_libraries(a2r4-menu PRIVATE a2r4_menu_engine)
set_target_properties(a2r4-menu PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
