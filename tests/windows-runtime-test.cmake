cmake_minimum_required(VERSION 3.16)

function(run_success)
  execute_process(COMMAND ${ARGV} RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "Command failed (${result}): ${ARGV}\n${output}\n${error}")
  endif()
endfunction()

function(expect_failure expected_message)
  execute_process(COMMAND ${ARGN} RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
  if(result EQUAL 0 OR NOT "${output}${error}" MATCHES "${expected_message}")
    message(FATAL_ERROR "Expected failure '${expected_message}': ${ARGN}\n${output}\n${error}")
  endif()
endfunction()

# The test owns this directory, and creates fresh inputs on every invocation.
set(work "${TEST_BINARY_DIR}/windows-runtime-fixture")
file(REMOVE_RECURSE "${work}")
set(lib_dir "${work}/vendor libs")
file(MAKE_DIRECTORY "${lib_dir}")
set(runtime_names sherpa-onnx-c-api.dll sherpa-onnx-cxx-api.dll onnxruntime.dll onnxruntime_providers_shared.dll)
foreach(dll IN LISTS runtime_names)
  file(WRITE "${lib_dir}/${dll}" "fixture ${dll}\n")
endforeach()
file(WRITE "${lib_dir}/sherpa-onnx-c-api.lib" "import library must not be installed\n")

run_success(
  "${CMAKE_COMMAND}" -S "${TEST_SOURCE_DIR}/windows-runtime" -B "${work}/build"
  "-DSHERPA_LIB_DIR=${lib_dir}"
)
run_success("${CMAKE_COMMAND}" --build "${work}/build" --config Release)
run_success("${CMAKE_COMMAND}" --install "${work}/build" --config Release --prefix "${work}/package")
set(
  verify_command
  "${CMAKE_COMMAND}"
  "-DPACKAGE_DIR=${work}/package"
  "-DSHERPA_LIB_DIR=${lib_dir}"
  -DPLUGIN_NAME=captionflow
  -P
  "${TEST_SOURCE_DIR}/../cmake/windows/verify-package.cmake"
)
run_success(${verify_command})
if(EXISTS "${work}/package/captionflow/bin/64bit/sherpa-onnx-c-api.lib")
  message(FATAL_ERROR "Import library leaked into the package")
endif()

# Cover the original omission as well as partial and stale dependency packages.
foreach(dll IN LISTS runtime_names)
  file(REMOVE "${work}/package/captionflow/bin/64bit/${dll}")
  expect_failure("Windows package is missing" ${verify_command})
  file(COPY "${lib_dir}/${dll}" DESTINATION "${work}/package/captionflow/bin/64bit")
endforeach()
file(WRITE "${work}/package/captionflow/bin/64bit/onnxruntime.dll" "stale runtime\n")
expect_failure("mismatched runtime DLL" ${verify_command})
file(REMOVE "${work}/package/captionflow/bin/64bit/onnxruntime.dll")
file(COPY "${lib_dir}/onnxruntime.dll" DESTINATION "${work}/package/captionflow/bin/64bit")
run_success(${verify_command})

# Verify the archive layout too, after a ZIP round trip.
run_success(
  "${CMAKE_COMMAND}" -E chdir "${work}/package" "${CMAKE_COMMAND}" -E tar cf "${work}/package.zip"
  --format=zip captionflow
)
file(MAKE_DIRECTORY "${work}/unpacked")
run_success("${CMAKE_COMMAND}" -E chdir "${work}/unpacked" "${CMAKE_COMMAND}" -E tar xf "${work}/package.zip")
run_success(
  "${CMAKE_COMMAND}" "-DPACKAGE_DIR=${work}/unpacked" "-DSHERPA_LIB_DIR=${lib_dir}"
  -DPLUGIN_NAME=captionflow -P "${TEST_SOURCE_DIR}/../cmake/windows/verify-package.cmake"
)

# A dependency cache with import libraries alone must fail at configure time.
file(REMOVE "${lib_dir}/sherpa-onnx-c-api.dll")
expect_failure(
  "Missing sherpa-onnx Windows runtime"
  "${CMAKE_COMMAND}" -S "${TEST_SOURCE_DIR}/windows-runtime" -B "${work}/missing-runtime-build"
  "-DSHERPA_LIB_DIR=${lib_dir}"
)
