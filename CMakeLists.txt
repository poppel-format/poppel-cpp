cmake_minimum_required(VERSION 3.18)

project(poppel VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Note: the trailing slash is important for cmake not to install in a nested "include" folder.
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ TYPE INCLUDE)

add_subdirectory(src)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_subdirectory(test)

    # Build tool specific
    if(MSVC)
        set_directory_properties(PROPERTY VS_STARTUP_PROJECT poppeltest)
    endif()

endif()
