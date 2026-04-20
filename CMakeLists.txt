cmake_minimum_required(VERSION 3.20)

project(gate-cli
  VERSION 0.1.0
  DESCRIPTION "CLI tooling for the Gate-Lang Hardware Description Language"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

include(FetchContent)

set(GATEO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  gateo-cpp
  GIT_REPOSITORY https://github.com/Rhodes-Gate-Lang/gateo-cpp.git
  GIT_TAG        v2.0.1
)
FetchContent_MakeAvailable(gateo-cpp)

# CLI11: header-only argument parser with first-class subcommand support.
# Docs: https://cliutils.github.io/CLI11/book/
FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG        v2.6.2
)
FetchContent_MakeAvailable(cli11)

add_library(gate_cli STATIC
  src/sim/Evaluation.cpp
  src/sim/NumberFormatter.cpp
  src/sim/SimConfig.cpp
  src/sim/Simulator.cpp
)

target_include_directories(gate_cli
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(gate_cli
  PUBLIC
    gateo::gateo
    CLI11::CLI11
)

add_executable(gate-lang-cli src/cli/GateCli.cpp)
set_target_properties(gate-lang-cli PROPERTIES
  OUTPUT_NAME gate
)
target_link_libraries(gate-lang-cli PRIVATE gate_cli)
