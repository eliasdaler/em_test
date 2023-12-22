add_executable(game
  main.cpp
)

set_target_properties(game PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
)

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

if(EMSCRIPTEN)
  target_link_options(game PRIVATE
    "SHELL:-s GL_ENABLE_GET_PROC_ADDRESS"
    "SHELL:-s MIN_WEBGL_VERSION=2"
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
