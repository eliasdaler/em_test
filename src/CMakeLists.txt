add_executable(game
  Graphics/Mesh.cpp

  util/GLUtil.cpp
  util/GltfLoader.cpp
  util/ImageLoader.cpp
  util/OSUtil.cpp

  Game.cpp
  main.cpp
)

target_include_directories(game PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(game PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
)

# SDL link
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

if (NOT EMSCRIPTEN)
  target_link_libraries(game PRIVATE glad::glad)
endif()

set(assets_dir "${PROJECT_SOURCE_DIR}/assets")
if (NOT EMSCRIPTEN)
  add_custom_target(copy_assets 
    COMMENT "Copying game assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${assets_dir}" "${CMAKE_CURRENT_BINARY_DIR}/assets"
  )

  add_dependencies(game copy_assets)
endif()

target_link_libraries(game PRIVATE 
  glm::glm
  stb::image
  tinygltf::tinygltf
)

target_compile_definitions(game
  PUBLIC
    GLM_FORCE_CTOR_INIT
    GLM_FORCE_XYZW_ONLY
    GLM_FORCE_EXPLICIT_CTOR
)

if(EMSCRIPTEN)
  target_link_options(game PRIVATE
    "SHELL:-s GL_ENABLE_GET_PROC_ADDRESS"
    "SHELL:-s MIN_WEBGL_VERSION=2"
    "SHELL:-s GL_EXPLICIT_UNIFORM_LOCATION=1"
    "SHELL:--preload-file ${assets_dir}@/assets"
  )

  set(site_dir "${CMAKE_CURRENT_BINARY_DIR}/site")

  set_target_properties(game PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${site_dir}"
    LIBRARY_OUTPUT_DIRECTORY "${site_dir}"
    RUNTIME_OUTPUT_DIRECTORY "${site_dir}"
  )

  set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

# web deploy/publishing
if(EMSCRIPTEN)
  include(WebBundle)
  include(ItchIoPublish)

  add_web_bundle_target(game)
  add_itch_io_publish_target(game "emscripten-test")
endif()
