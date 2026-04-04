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
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

find_package(ZLIB REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

if(WIN32)
    add_executable(tenebrarium WIN32 ${SOURCES})
else()
    add_executable(tenebrarium ${SOURCES})
endif()

target_include_directories(tenebrarium PRIVATE
    src
)

target_link_libraries(tenebrarium PRIVATE
    raylib
    ZLIB::ZLIB
)

target_compile_options(tenebrarium PRIVATE -Wall -Wextra)

# Symlink assets/ dentro del directorio de build para poder ejecutar desde ahí
add_custom_command(TARGET tenebrarium POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/assets
    COMMENT "Enlazando assets al directorio de build"
)

