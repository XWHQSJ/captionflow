cmake_minimum_required(VERSION 3.16)
project(windows-runtime-fixture LANGUAGES C)

# Synthetic files exercise real CMake build/install rules on any host, offline.
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/windows/sherpa-runtime.cmake")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/plugin.c" "int captionflow_fixture(void) { return 0; }\n")
add_library(captionflow MODULE "${CMAKE_CURRENT_BINARY_DIR}/plugin.c")
set_target_properties(
  captionflow
  PROPERTIES PREFIX "" SUFFIX ".dll" WINDOWS_EXPORT_ALL_SYMBOLS ON
)
install(TARGETS captionflow DESTINATION "captionflow/bin/64bit")
install_sherpa_windows_runtime(captionflow "${SHERPA_LIB_DIR}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/en-US.ini" "fixture\n")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/en-US.ini" DESTINATION "captionflow/data/locale")
