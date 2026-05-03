cmake_minimum_required (VERSION 3.10)
project (LINENOISE DESCRIPTION "A shell library")

# Configuration specific to being the main project
if(PROJECT_NAME STREQUAL CMAKE_PROJECT_NAME)
    set(CMAKE_VERBOSE_MAKEFILE ON)
    set(LINENOISE_NOT_SUBPROJECT ON)
endif(PROJECT_NAME STREQUAL CMAKE_PROJECT_NAME)

if(CMAKE_BUILD_TYPE)
    message("Building ${PROJECT_NAME} in ${CMAKE_BUILD_TYPE} build.")
elseif(NOT CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "No build type specified. Use -DCMAKE_BUILD_TYPE=[DEBUG|RELEASE] to specify.")
endif(CMAKE_BUILD_TYPE)

option(LINENOISE_BUILD_EXAMPLES "Build examples" OFF)
option(LINENOISE_BUILD_TESTS "Build unit tests" OFF)

# set the version of our library
# MAJOR, is significant breaking changes, MINOR, is when we might have changed signatures of existing functions, PATCH, is just bug fixes and no breaking changes
set(LINENOISE_VERSION_MAJOR 1)
set(LINENOISE_VERSION_MINOR 0)
set(LINENOISE_VERSION_PATCH 1)
set(LINENOISE_VERSION_STRING ${LINENOISE_VERSION_MAJOR}.${LINENOISE_VERSION_MINOR}.${LINENOISE_VERSION_PATCH})

file(GLOB LINENOISE_SOURCE_FILES "linenoise.c")
file(GLOB LINENOISE_HEADER_FILES "linenoise.h")

set(LINENOISE_SOURCES ${LINENOISE_SOURCE_FILES})
set(LINENOISE_HEADERS ${LINENOISE_HEADER_FILES})

# where the library will be built to
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/libs)

add_library(linenoise SHARED ${LINENOISE_SOURCES} ${LINENOISE_HEADERS})
target_link_libraries(linenoise)

target_include_directories(linenoise PUBLIC ".")
# sets the shared library version
# http://cmake.3232098.n2.nabble.com/Version-in-name-of-shared-library-td7581530.html
set_target_properties(linenoise PROPERTIES VERSION ${LINENOISE_VERSION_STRING} SOVERSION ${LINENOISE_VERSION_MAJOR})
set_target_properties(linenoise PROPERTIES LINKER_LANGUAGE C)

# create pkg-config files for configuration in non-cmake projects
# configure_file(linenoise.pc.in linenoise.pc @ONLY)

# only offer install option if they compiled with release mode
if(CMAKE_BUILD_TYPE MATCHES "^(RELEASE|Release)")
	# adds the 'make install' targets to copy the shared library to /usr/local/lib/ directory
	install(TARGETS linenoise DESTINATION ${CMAKE_INSTALL_LIBDIR} )
	# adds all the relevant headers to /usr/local/include/gphoto2pp when ``make install`` is executed
	install(FILES ${LINENOISE_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/linenoise )
	# install the pkg-config config to system folder
	install(FILES ${CMAKE_BINARY_DIR}/linenoise.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
endif(CMAKE_BUILD_TYPE MATCHES "^(RELEASE|Release)")

if(LINENOISE_BUILD_EXAMPLES)
    message(STATUS "Building examples")
    add_executable(example example.c)
    target_link_libraries(example linenoise)
endif(LINENOISE_BUILD_EXAMPLES)

if(LINENOISE_BUILD_TESTS)
    message(STATUS "Building tests")
    add_subdirectory(test)
endif(LINENOISE_BUILD_TESTS)