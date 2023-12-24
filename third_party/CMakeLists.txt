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

## tinygltf
option(TINYGLTF_BUILD_LOADER_EXAMPLE "Build loader_example(load glTF and dump infos)" OFF)
option(TINYGLTF_INSTALL "Install tinygltf files during install step." OFF)
option(TINYGLTF_HEADER_ONLY "On: header-only mode. Off: create tinygltf library(No TINYGLTF_IMPLEMENTATION required in your project)" ON)
add_subdirectory(tinygltf)
add_library(tinygltf::tinygltf ALIAS tinygltf)

## glm
add_subdirectory(glm)

### Dear ImGui
set(IMGUI_SOURCES
  "imgui.cpp"
  "imgui_draw.cpp"
  "imgui_tables.cpp"
  "imgui_widgets.cpp"
  "imgui_demo.cpp"
  # C++ additions
  "misc/cpp/imgui_stdlib.cpp"
  # SDL backend
  "backends/imgui_impl_opengl3.cpp"
  "backends/imgui_impl_sdl2.cpp"
)

list(TRANSFORM IMGUI_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/imgui/")

add_library(imgui_sdl STATIC ${IMGUI_SOURCES})
set_property(TARGET imgui_sdl PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(imgui_sdl
  PUBLIC
    "imgui"
    "imgui/backends"
)

if(BUILD_SHARED_LIBS)
  target_link_libraries(imgui_sdl
    PUBLIC
      SDL2::SDL2
  )
else()
  target_link_libraries(imgui_sdl
    PUBLIC
      SDL2::SDL2-static
  )
endif()
add_library(imgui::imgui ALIAS imgui_sdl)

