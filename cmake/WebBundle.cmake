function (add_web_bundle_target target_name)
  set(index_page_path "${PROJECT_SOURCE_DIR}/html/index.html")
  set(web_output_dir "${CMAKE_CURRENT_BINARY_DIR}/site")

  add_custom_command(TARGET ${target_name}
    POST_BUILD
    COMMENT "Bundling website dir"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${index_page_path}" "${web_output_dir}"
  )
endfunction()
