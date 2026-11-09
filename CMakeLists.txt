cmake_minimum_required(VERSION 3.25)
project(a2r4 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The race (spel.dat lifted with SwARc) comes later, see CLAUDE.md.

# Native menu (replaces MenuData/Menu.exe), see src/menu/.
add_subdirectory(src/menu)
