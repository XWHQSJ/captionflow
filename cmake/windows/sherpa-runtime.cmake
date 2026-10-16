# Keep build-tree and installed Windows runtime dependencies in sync.
include_guard(GLOBAL)

function(install_sherpa_windows_runtime target lib_dir)
  file(GLOB runtime_dlls CONFIGURE_DEPENDS "${lib_dir}/*.dll")
  # An import library is enough to link, but not enough to load the plugin.
  foreach(required_dll sherpa-onnx-c-api.dll onnxruntime.dll)
    if(NOT EXISTS "${lib_dir}/${required_dll}")
      message(FATAL_ERROR "Missing sherpa-onnx Windows runtime: ${lib_dir}/${required_dll}")
    endif()
  endforeach()

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${runtime_dlls} "$<TARGET_FILE_DIR:${target}>"
    COMMAND_EXPAND_LISTS
    VERBATIM
  )
  # Release archives contain the install tree, not TARGET_FILE_DIR.
  install(FILES ${runtime_dlls} DESTINATION "${target}/bin/64bit")
endfunction()
