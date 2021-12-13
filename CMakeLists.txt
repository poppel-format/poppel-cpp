cmake_minimum_required(VERSION 3.18)

project(poppel VERSION 0.0.0 LANGUAGES CXX)

# Note: the trailing slash is important for cmake not to install in a nested "include" folder.
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ TYPE INCLUDE)

# Configure external dependencies.
find_package(nlohmann_json CONFIG REQUIRED)
