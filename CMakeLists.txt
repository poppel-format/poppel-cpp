cmake_minimum_required(VERSION 3.18)

project(poppel VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Note: the trailing slash is important for cmake not to install in a nested "include" folder.
set(POPPEL_VERSION                0.0.1)
set(POPPEL_INCLUDE_DIR            "${PROJECT_SOURCE_DIR}/include/")
set(POPPEL_INCLUDE_INSTALL_DIR    "${CMAKE_INSTALL_INCLUDEDIR}")
set(POPPEL_CONFIG_INSTALL_DIR     "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" CACHE INTERNAL "")
set(POPPEL_TARGETS_EXPORT_NAME    "${PROJECT_NAME}Targets")
set(POPPEL_CMAKE_TARGETS_FILE     "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")
set(POPPEL_CMAKE_CONFIG_FILE      "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake")
set(POPPEL_CMAKE_CONFIG_VERSION_FILE  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake")

include(CMakePackageConfigHelpers)

add_subdirectory(src)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_subdirectory(test)

    # Build tool specific
    if(MSVC)
        set_directory_properties(PROPERTY VS_STARTUP_PROJECT poppeltest)
    endif()

endif()

# Dependencies.
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(poppel PRIVATE nlohmann_json nlohmann_json::nlohmann_json)


if(poppel_install)
    install(DIRECTORY ${POPPEL_INCLUDE_DIR} TYPE INCLUDE)

    export(
        TARGETS poppel
        NAMESPACE ${PROJECT_NAME}::
        FILE ${POPPEL_CMAKE_TARGETS_FILE}
    )
    # Generate target file export.
    install(
        TARGETS poppel
        EXPORT ${POPPEL_TARGETS_EXPORT_NAME}
        INCLUDES DESTINATION ${POPPEL_INCLUDE_INSTALL_DIR}
    )
    # Install the target export file.
    install(
        EXPORT ${POPPEL_TARGETS_EXPORT_NAME}
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${POPPEL_CONFIG_INSTALL_DIR}
    )

    # Generate config files.
    configure_package_config_file(
        cmake/config.cmake.in
        ${POPPEL_CMAKE_CONFIG_FILE}
        INSTALL_DESTINATION ${POPPEL_CONFIG_INSTALL_DIR}
        NO_SET_AND_CHECK_MACRO
    )
    write_basic_package_version_file(
        ${POPPEL_CMAKE_CONFIG_VERSION_FILE}
        VERSION ${POPPEL_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    # Install the config files.
    install(
        FILES ${POPPEL_CMAKE_CONFIG_FILE} ${POPPEL_CMAKE_CONFIG_VERSION_FILE}
        DESTINATION ${POPPEL_CONFIG_INSTALL_DIR}
    )
endif()
