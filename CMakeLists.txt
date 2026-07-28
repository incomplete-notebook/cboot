cmake_minimum_required(VERSION 3.13)
project(cboot C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Executable module
add_subdirectory(main)
add_subdirectory(domain)
add_subdirectory(commands)
add_subdirectory(parser)
add_subdirectory(generator)
add_subdirectory(docgen)
add_subdirectory(typecheck)
add_subdirectory(utils)

# Add normal module sources to executable
target_sources(main PRIVATE ${domain_SOURCES})
target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/domain)
target_sources(main PRIVATE ${commands_SOURCES})
target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/commands)
target_sources(main PRIVATE ${parser_SOURCES})
target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/parser)
target_sources(main PRIVATE ${generator_SOURCES})
target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/generator)
target_sources(main PRIVATE ${docgen_SOURCES})
target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/docgen)
target_sources(main PRIVATE ${typecheck_SOURCES})
target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/typecheck)
target_sources(main PRIVATE ${utils_SOURCES})
target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/utils)

# Link library modules into executable

target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR})

# End of CMakeLists.txt
