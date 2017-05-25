cmake_minimum_required(VERSION 3.0)

project(linenoise)

set(SOURCES linenoise.c)

add_library(linenoise SHARED ${SOURCES})
add_library(linenoise-static STATIC ${SOURCES})
add_executable(linenoise_example example.c)
target_link_libraries(linenoise_example linenoise)

install (TARGETS linenoise DESTINATION lib)
install (TARGETS linenoise-static DESTINATION lib)
install (FILES linenoise.h DESTINATION include)
