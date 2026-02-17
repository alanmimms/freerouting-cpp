cmake_minimum_required(VERSION 3.25)
project(freerouting-cpp VERSION 0.1.0 LANGUAGES CXX)

# C++23 required for std::flat_map, std::mdspan, std::jthread improvements
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler requirements
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13.0)
    message(FATAL_ERROR "GCC 13+ required for C++23 features")
  endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 17.0)
    message(FATAL_ERROR "Clang 17+ required for C++23 features")
  endif()
endif()

# Compiler warnings
add_compile_options(
  -Wall
  -Wextra
  -Wpedantic
  $<$<CONFIG:Debug>:-g>
  $<$<CONFIG:Release>:-O3>
)

# Project directories
set(FREEROUTING_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(FREEROUTING_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Find SDL2
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# Main library
add_library(freerouting
  src/autoroute/ExpansionDoor.cpp
  src/autoroute/FreeSpaceExpansionRoom.cpp
  src/autoroute/CompleteFreeSpaceExpansionRoom.cpp
  src/autoroute/TargetItemExpansionDoor.cpp
  src/autoroute/DestinationDistance.cpp
  src/autoroute/AutorouteEngine.cpp
  src/autoroute/MazeSearchAlgo.cpp
  src/autoroute/SimpleGridRouter.cpp
  src/autoroute/ExpansionRoomGenerator.cpp
  src/autoroute/BatchAutorouter.cpp
  src/autoroute/PushAndShove.cpp
  src/board/DrcEngine.cpp
  src/cli/CommandLineArgs.cpp
  src/io/DsnReader.cpp
  src/io/DsnBoardConverter.cpp
  src/visualization/CongestionHeatmap.cpp
  src/visualization/BoardRenderer.cpp
)
target_include_directories(freerouting PUBLIC ${FREEROUTING_INCLUDE_DIR} ${SDL2_INCLUDE_DIRS})
target_link_libraries(freerouting PUBLIC pthread ${SDL2_LIBRARIES})

# Main executable
add_executable(freerouting-cli
  src/main.cpp
)
target_link_libraries(freerouting-cli PRIVATE freerouting)

# Testing with Catch2
enable_testing()
include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)

# Test executable
add_executable(freerouting_tests
  tests/test_main.cpp
  tests/test_geometry.cpp
  tests/test_rules.cpp
  tests/test_io.cpp
  tests/test_board.cpp
  tests/test_shapes.cpp
  tests/test_routing.cpp
  tests/test_expansion_rooms.cpp
  tests/test_maze_search.cpp
  tests/test_batch_autorouter.cpp
  tests/test_board_extensions.cpp
  tests/test_rule_areas.cpp
  tests/test_drc.cpp
  tests/test_data_structures.cpp
  tests/test_interactive_routing.cpp
  tests/test_phase5_geometry.cpp
)
target_link_libraries(freerouting_tests PRIVATE freerouting Catch2::Catch2WithMain)

# Register tests with CTest
include(CTest)
include(Catch)
catch_discover_tests(freerouting_tests)
