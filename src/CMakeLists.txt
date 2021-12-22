
file(GLOB_RECURSE poppel_src_list LIST_DIRECTORIES true CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_library(poppellib ${poppel_src_list})

# Include paths.
target_include_directories(poppellib PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/../include
    ${CMAKE_CURRENT_LIST_DIR}
)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(poppellib PRIVATE nlohmann_json nlohmann_json::nlohmann_json)
