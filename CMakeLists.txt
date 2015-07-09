
# Default cmake version
#
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.1)

# Project name
#
PROJECT(linenoise)

# Compile in release mode by default
#
IF(NOT CMAKE_BUILD_TYPE)
    MESSAGE(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
    SET(CMAKE_BUILD_TYPE RelWithDebInfo) 
ELSE(NOT CMAKE_BUILD_TYPE)
    MESSAGE(STATUS "Using specified '${CMAKE_BUILD_TYPE}' build type.")
ENDIF(NOT CMAKE_BUILD_TYPE)

# Build output checks
#
OPTION(LINENOISE_BUILD_SHARED "Build LINENOISE as a shared library." ON)
IF(LINENOISE_BUILD_SHARED)
    MESSAGE(
        STATUS 
        "Build will generate a shared library. "
        "Use LINENOISE_BUILD_SHARED=0 to perform a static build"
    )
    SET(LINENOISE_TYPE SHARED)
ELSE(LINENOISE_BUILD_SHARED)
    MESSAGE(STATUS "Build will generate a static library.")
    SET(LINENOISE_TYPE STATIC)
ENDIF(LINENOISE_BUILD_SHARED)

# The version number.
#
SET(LINENOISE_VERSION_MAJOR 1)
SET(LINENOISE_VERSION_MINOR 0)
SET(LINENOISE_VERSION "${LINENOISE_VERSION_MAJOR}.${LINENOISE_VERSION_MINOR}")

# The library
#
add_library(linenoise ${LINENOISE_TYPE} linenoise.c linenoise.h)

# The test case
#
add_executable(example example.c)
target_link_libraries(example linenoise)

# Install
#
install(TARGETS linenoise DESTINATION lib)

