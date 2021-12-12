cmake_minimum_required(VERSION 3.18)

project(poppel VERSION 0.0.0 LANGUAGES CXX)

# Configure external dependencies.
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(poppel PRIVATE nlohmann_json nlohmann_json::nlohmann_json)
