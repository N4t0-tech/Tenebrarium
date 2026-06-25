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
set(CMAKE_WARN_DEPRECATION OFF)
FetchContent_MakeAvailable(raylib)
set(CMAKE_WARN_DEPRECATION ON)

find_package(ZLIB REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

execute_process(
    COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(GIT_VERSION STREQUAL "" OR GIT_VERSION MATCHES "^[0-9a-f]+$")
    set(GIT_VERSION "0.0.0")
endif()
string(REGEX REPLACE "^v" "" RAW_VERSION "${GIT_VERSION}")
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" VERSION_BASE "${RAW_VERSION}")
if(NOT VERSION_BASE)
    set(VERSION_BASE "0.0.0")
endif()
set(GIT_VERSION_STRING "${VERSION_BASE}")
string(REPLACE "." "," GIT_VERSION_COMMA "${VERSION_BASE}")
string(APPEND GIT_VERSION_COMMA ",0")

set(ICON_PATH "${CMAKE_SOURCE_DIR}/assets/tenebrarium.ico")
set(MANIFEST_PATH "${CMAKE_SOURCE_DIR}/src/win_resources.manifest")
configure_file(
    src/win_resources.rc.in
    ${CMAKE_BINARY_DIR}/generated/win_resources.rc
    @ONLY
)

if(WIN32)
    add_executable(tenebrarium WIN32 ${SOURCES} ${CMAKE_BINARY_DIR}/generated/win_resources.rc)
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

