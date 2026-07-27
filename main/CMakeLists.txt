# CMakeLists.txt for module main (CBoot generated, compiler: exe)

set(main_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
)


# Executable target
add_executable(main ${main_SOURCES})

target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
