cmake_minimum_required(VERSION 3.18)

project(
    emscripten-demo
    VERSION 0.1.0
    LANGUAGES CXX C
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(ITCH_USERNAME "eliasdaler" CACHE STRING "itch.io username")

# Check that git submodules were cloned
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL/CMakeLists.txt")
  include(FetchSubmodules)
  fetch_submodules()
endif()

add_subdirectory(third_party)
add_subdirectory(src)
