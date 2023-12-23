### SDL
if (NOT BUILD_SHARED_LIBS)
  set(SDL_SHARED_ENABLED_BY_DEFAULT OFF CACHE BOOL "Don't build SDL as shared lib")
endif()
option(SDL_TEST "Build the SDL2_test library" OFF)
option(SDL_AUDIO_ENABLED_BY_DEFAULT "Enable the Audio subsystem" OFF)
add_subdirectory(SDL)

### stb
add_subdirectory(stb)

## glad
if (NOT EMSCRIPTEN)
add_library(glad STATIC "${CMAKE_CURRENT_LIST_DIR}/glad/src/gl.c")
target_include_directories(glad PUBLIC "${CMAKE_CURRENT_LIST_DIR}/glad/include")
add_library(glad::glad ALIAS glad)
set_target_properties(glad PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
endif()
