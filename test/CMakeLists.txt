cmake_minimum_required(VERSION 3.18)

add_executable(poppeltest main.cpp)

# Include paths.
target_include_directories(poppeltest PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/../include
    ${CMAKE_CURRENT_LIST_DIR}/..
)

target_link_libraries(poppeltest PRIVATE poppellib)

find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(poppeltest PRIVATE Catch2::Catch2)

add_test(NAME poppeltest COMMAND poppeltest)
