
file(GLOB_RECURSE poppel_src_list LIST_DIRECTORIES true CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_library(poppel STATIC ${poppel_src_list})

# Include paths.
target_include_directories(poppel PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:include>
)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(poppel PRIVATE nlohmann_json nlohmann_json::nlohmann_json)
