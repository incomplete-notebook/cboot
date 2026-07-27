cmake_minimum_required(VERSION 3.10)
project(cboot C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Module subdirectories
add_subdirectory(src/domain)
add_subdirectory(src/commands)
add_subdirectory(src/parser)
add_subdirectory(src/generator)
add_subdirectory(src/docgen)
add_subdirectory(src/typecheck)
add_subdirectory(src/utils)

# Collect all module sources
set(ALL_SOURCES src/main.c)
list(APPEND ALL_SOURCES src/domain/domain.c)
list(APPEND ALL_SOURCES src/commands/commands.c)
list(APPEND ALL_SOURCES src/parser/parser.c)
list(APPEND ALL_SOURCES src/generator/generator.c)
list(APPEND ALL_SOURCES src/docgen/docgen.c)
list(APPEND ALL_SOURCES src/typecheck/typecheck.c)
list(APPEND ALL_SOURCES src/utils/utils.c)

# Main executable
add_executable(cboot ${ALL_SOURCES})

target_include_directories(cboot PRIVATE ${CMAKE_SOURCE_DIR}/src)

# End of CMakeLists.txt
