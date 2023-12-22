function (add_itch_io_publish_target target_name itch_path)
  if (NOT ITCH_USERNAME) 
    message(FATAL_ERROR "ITCH_USERNAME was not set")
  endif()

  set(site_dir "${CMAKE_CURRENT_BINARY_DIR}/site")

  add_custom_target(itch_push_${target_name}
    COMMENT "Pushing to itch.io"
    COMMAND butler push --if-changed  ${site_dir} ${ITCH_USERNAME}/${itch_path}:web
    USES_TERMINAL
  )
  add_dependencies(itch_push_${target_name} ${target_name})
endfunction()
