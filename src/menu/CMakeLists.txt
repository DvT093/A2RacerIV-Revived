# a2r4-menu: native replacement for A2 Racer IV's Delphi menu (MenuData/Menu.exe).
#   a2r4/  A2 Racer IV screens and flow
# The menu engine (DFM, TGDI components, painter, video) and the race
# launcher are shared with Vakantie Racer and Europe Racer in
# ../Davilex-RED/menu (see its MIGRATION.md).
set(DAVILEX_RED_DIR ${CMAKE_SOURCE_DIR}/../Davilex-RED CACHE PATH "Shared Davilex RED code")
include(${DAVILEX_RED_DIR}/menu/davi_menu.cmake)

file(GLOB A2R4_MENU_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/a2r4/*.cpp)
add_executable(a2r4-menu ${A2R4_MENU_SOURCES})
target_include_directories(a2r4-menu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(a2r4-menu PRIVATE davi_menu_engine davi_menu_launcher)
set_target_properties(a2r4-menu PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
