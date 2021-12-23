cmake_minimum_required(VERSION 3.18)

project(poppel VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Note: the trailing slash is important for cmake not to install in a nested "include" folder.
set(POPPEL_INCLUDE_DIR            "${PROJECT_SOURCE_DIR}/include/")
set(POPPEL_INCLUDE_INSTALL_DIR    "${CMAKE_INSTALL_INCLUDEDIR}")
set(POPPEL_CONFIG_INSTALL_DIR     "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" CACHE INTERNAL "")
set(POPPEL_TARGETS_EXPORT_NAME    "${PROJECT_NAME}Targets")
set(POPPEL_CMAKE_TARGETS_FILE     "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")

add_subdirectory(src)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_subdirectory(test)

    # Build tool specific
    if(MSVC)
        set_directory_properties(PROPERTY VS_STARTUP_PROJECT poppeltest)
    endif()

endif()

if(poppel_install)
    # Note: the trailing slash is important for cmake not to install in a nested "include" folder.
    install(DIRECTORY ${POPPEL_INCLUDE_DIR} TYPE INCLUDE)

    export(
        TARGETS poppel
        NAMESPACE ${PROJECT_NAME}::
        FILE ${POPPEL_CMAKE_TARGETS_FILE}
    )
    install(
        TARGETS poppel
        EXPORT ${POPPEL_TARGETS_EXPORT_NAME}
        INCLUDES DESTINATION ${POPPEL_INCLUDE_INSTALL_DIR}
    )
    install(
        EXPORT ${POPPEL_TARGETS_EXPORT_NAME}
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${POPPEL_CONFIG_INSTALL_DIR}
    )
endif()
