cmake_minimum_required(VERSION 3.28)
project(snapper-studio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Test)
find_package(KDDockWidgets-qt6 2.4 REQUIRED)

# Warnings are errors, and the code runs without exceptions or RTTI.
add_library(snapper_options INTERFACE)
target_compile_options(snapper_options INTERFACE
  -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror
  -fno-exceptions -fno-rtti)
target_compile_definitions(snapper_options INTERFACE
  QT_NO_KEYWORDS QT_NO_CAST_FROM_ASCII)

# Everything except main() lives here, so tests link the same code.
add_library(snapper_core STATIC)
add_subdirectory(src)
target_include_directories(snapper_core PUBLIC src)
target_link_libraries(snapper_core PUBLIC
  snapper_options Qt6::Widgets KDAB::kddockwidgets)

qt_add_resources(snapper_core "theme" PREFIX "/theme" BASE theme
  FILES theme/theme.json)

qt_add_executable(snapper-studio src/main.cpp)
target_link_libraries(snapper-studio PRIVATE snapper_core)

enable_testing()
add_subdirectory(tests)

