cmake_minimum_required(VERSION 3.20)
project(Tenebrarium VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib
  GIT_TAG 5.0
  GIT_SHALLOW TRUE
  PATCH_COMMAND ${CMAKE_COMMAND} -E echo "Patching CMakeLists"
    COMMAND sed -i "s/cmake_minimum_required(VERSION 3.0)/cmake_minimum_required(VERSION 3.5)/" CMakeLists.txt || true
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

find_package(ZLIB REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(tenebrarium ${SOURCES})

target_include_directories(tenebrarium PRIVATE
    src
)

target_link_libraries(tenebrarium PRIVATE
    raylib
    ZLIB::ZLIB
)

target_compile_options(tenebrarium PRIVATE -Wall -Wextra)

target_compile_definitions(tenebrarium PRIVATE
    ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
)
