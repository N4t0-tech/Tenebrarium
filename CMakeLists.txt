cmake_minimum_required(VERSION 3.20)
project(Tenebrarium VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG v5.0.0
)
FetchContent_MakeAvailable(ftxui)

find_package(ZLIB REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(tenebrarium ${SOURCES})

target_include_directories(tenebrarium PRIVATE
    src
)

target_link_libraries(tenebrarium PRIVATE
    ftxui::screen
    ftxui::dom
    ftxui::component
    ZLIB::ZLIB
)

target_compile_options(tenebrarium PRIVATE -Wall -Wextra -Wpedantic)

target_compile_definitions(tenebrarium PRIVATE
    ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
)
