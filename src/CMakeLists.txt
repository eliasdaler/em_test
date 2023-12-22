add_executable(game
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

set(assets_dir "${PROJECT_SOURCE_DIR}/assets")
if (NOT EMSCRIPTEN)
  add_custom_target(copy_assets 
    COMMENT "Copying game assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${assets_dir}" "${CMAKE_CURRENT_BINARY_DIR}/assets"
  )

  add_dependencies(game copy_assets)
endif()

target_link_libraries(game PRIVATE stb::image)

if(EMSCRIPTEN)
  target_link_options(game PRIVATE
    "SHELL:-s GL_ENABLE_GET_PROC_ADDRESS"
    "SHELL:-s MIN_WEBGL_VERSION=2"
    "SHELL:--preload-file ${assets_dir}@/assets"
  )

  set(site_dir "${CMAKE_CURRENT_BINARY_DIR}/site")

  set_target_properties(game PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${site_dir}"
    LIBRARY_OUTPUT_DIRECTORY "${site_dir}"
    RUNTIME_OUTPUT_DIRECTORY "${site_dir}"
  )
endif()

# web deploy/publishing
if(EMSCRIPTEN)
  include(WebBundle)
  include(ItchIoPublish)

  add_web_bundle_target(game)
  add_itch_io_publish_target(game "emscripten-test")
endif()
