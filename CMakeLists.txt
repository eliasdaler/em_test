cmake_minimum_required(VERSION 3.18)

project(
    game
    VERSION 0.1.0
    LANGUAGES CXX C
)

add_executable(game
  main.cpp
)

if(EMSCRIPTEN)
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()
