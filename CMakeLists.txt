cmake_minimum_required(VERSION 2.8)

project(LiteJit C ASM)

set(CMAKE_C_FLAGS_DEBUG "-m32 -Wall -Wextra -std=c99 -I${CMAKE_CURRENT_SOURCE_DIR}/include -O0 -g -ggdb -DLJIT_DEBUG")

set(CMAKE_C_FLAGS_RELEASE "-Wall -Wextra -std=c99 -I${CMAKE_CURRENT_SOURCE_DIR}/include -O2 -s -DNDEBUG")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_DEBUG}")

add_subdirectory(tool)

SET(src_x86 "${CMAKE_CURRENT_SOURCE_DIR}/src/core/x86-code.inc.in")
SET(dst_x86 "${CMAKE_CURRENT_SOURCE_DIR}/src/core/x86-code.inc")

add_custom_command(OUTPUT "${dst_x86}"
                   DEPENDS "${src_x86}" codegen
                   COMMAND
                   "./tool/codegen" ${src_x86} ${dst_x86}
                   )

set(ljit_src
            src/core/internal.c
            src/core/instance.c
            src/core/function.c
            src/core/types.c
            src/core/bytecode.c
            src/core/bytecode-dumper.c
            src/core/bytecode-list.c
            src/core/block.c
            src/core/label.c
            src/core/codegen.c
            src/core/regalloc.c
            src/core/compile.c
            src/core/arch.c
            src/core/apply.c
            src/core/flow-graph.c
            src/core/liveness.c
            src/core/liveness-utils.c
            src/core/interference.c
            src/core/color-stack.c
            src/core/color.c

            src/elf/elf.c
            src/elf/header.c
            src/elf/section.c
            src/elf/write.c
            )
## Add debug files if debug mode is set
if (${CMAKE_C_FLAGS} MATCHES ${CMAKE_C_FLAGS_DEBUG})
    set(ljit_src ${ljit_src} src/elf/debug.c)
endif()

ADD_LIBRARY(litejit_lib SHARED ${ljit_src})
add_custom_target(x86_code DEPENDS "${dst_x86}")
add_dependencies(litejit_lib x86_code)
TARGET_LINK_LIBRARIES(litejit_lib ffi)

SET_TARGET_PROPERTIES(
    litejit_lib
    PROPERTIES
    VERSION 0.0.1
    OUTPUT_NAME litejit
)

# Test rule (make test)

enable_testing()

add_subdirectory(test)

# Documentation rule (make doc)

find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
                     )
endif(DOXYGEN_FOUND)

# Clean working directory (make distclean)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_clear.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_clear.cmake"
  IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(distclean
  COMMAND ${CMAKE_BUILD_TOOL} clean
  COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_clear.cmake")
