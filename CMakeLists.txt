cmake_minimum_required(VERSION 3.4)
project(linenoise C)

include(GNUInstallDirs)

option(LINENOISE_BUILD_EXAMPLE OFF)

# -----------------
# LIBRARY
# -----------------
add_library(${PROJECT_NAME} linenoise.c)
target_include_directories(${PROJECT_NAME}
    INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/linenoise>
)
target_compile_options(${PROJECT_NAME}
    PRIVATE -Wall -W
)

# -----------------
# EXAMPLE
# -----------------
if(LINENOISE_BUILD_EXAMPLE)
    add_executable(${PROJECT_NAME}-example example.c)
    target_link_libraries(${PROJECT_NAME}-example
        PRIVATE ${PROJECT_NAME}
    )
    target_compile_options(${PROJECT_NAME}
        PRIVATE -Wall -W
    )
endif()

# -----------------
# INSTALLATION
# -----------------
if(TARGET ${PROJECT_NAME}-example)
    install (TARGETS ${PROJECT_NAME}-example
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install (TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Config
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/linenoise.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/linenoise/
)

install(EXPORT ${PROJECT_NAME}Config
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
