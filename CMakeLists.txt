cmake_minimum_required(VERSION 3.18)

project(
    game
    VERSION 0.1.0
    LANGUAGES CXX C
)

add_subdirectory(third_party)

add_executable(game
  main.cpp
)

set_target_properties(game PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
)

if(EMSCRIPTEN)
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s GL_ENABLE_GET_PROC_ADDRESS -sMIN_WEBGL_VERSION=2")
endif()

if(BUILD_SHARED_LIBS)
  target_link_libraries(game PRIVATE
    SDL2::SDL2
  )
else()
  target_link_libraries(game PRIVATE
    SDL2::SDL2-static
  )
endif()

if(WIN32)
  target_link_libraries(game PRIVATE SDL2::SDL2main)
endif() 
