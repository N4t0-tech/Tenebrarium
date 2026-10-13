execute_process(
    COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY ${SOURCE_DIR}
    OUTPUT_VARIABLE GAME_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(GAME_VERSION STREQUAL "")
    set(GAME_VERSION "desconocida")
endif()

set(CONTENT "#pragma once\n\ninline constexpr const char* kGameVersion = \"${GAME_VERSION}\";\n")
if(EXISTS ${OUTPUT_FILE})
    file(READ ${OUTPUT_FILE} OLD_CONTENT)
endif()
if(NOT "${CONTENT}" STREQUAL "${OLD_CONTENT}")
    file(WRITE ${OUTPUT_FILE} "${CONTENT}")
endif()
