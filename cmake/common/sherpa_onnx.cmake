# Downloads and extracts the sherpa-onnx prebuilt tarball declared in
# buildspec.json under the `sherpa-onnx` dependency, for the current OS slice.
#
# The archive is extracted into `${CMAKE_SOURCE_DIR}/.deps/sherpa-onnx/` and the
# `include/` + `lib/` directories live directly inside it so the top-level
# CMakeLists can find SherpaOnnx::CApi there.
#
# Expected schema in buildspec.json:
#   "sherpa-onnx": {
#     "version": "1.12.39",
#     "versions": { "macos": "1.13.8" },  // optional per-platform override
#     "baseUrl": "https://github.com/k2-fsa/sherpa-onnx/releases/download",
#     "assets":  { "macos": "sherpa-onnx-v1.12.39-macos-universal-shared.tar.bz2",
#                  "windows-x64": "sherpa-onnx-v1.12.39-win-x64-shared.tar.bz2" },
#     "hashes":  { "macos": "<sha256>", "windows-x64": "<sha256>" }
#   }

include_guard(GLOBAL)

function(_fetch_sherpa_onnx platform_label)
  set(dest "${CMAKE_SOURCE_DIR}/.deps/sherpa-onnx")

  if(NOT platform_label STREQUAL "macos" AND EXISTS "${dest}/include/sherpa-onnx/c-api/c-api.h")
    # Already extracted — allow re-runs to be cheap.
    message(STATUS "sherpa-onnx already present at ${dest}, skipping download")
    return()
  endif()

  file(READ "${CMAKE_SOURCE_DIR}/buildspec.json" buildspec)
  string(JSON sherpa GET "${buildspec}" dependencies "sherpa-onnx")
  string(JSON version GET "${sherpa}" version)
  if(platform_label STREQUAL "macos")
    string(JSON version GET "${sherpa}" versions macos)
  endif()
  string(JSON base_url GET "${sherpa}" baseUrl)
  string(JSON asset GET "${sherpa}" assets "${platform_label}")
  string(JSON expected_hash ERROR_VARIABLE hash_err GET "${sherpa}" hashes "${platform_label}")

  if(platform_label STREQUAL "macos")
    set(pin_stamp "${dest}/.captionflow-macos-pin")
    set(pin_key "${version}:${asset}:${expected_hash}")
    if(NOT expected_hash)
      message(FATAL_ERROR "The macOS sherpa-onnx archive must have a SHA256 pin")
    endif()
    if(EXISTS "${pin_stamp}" AND EXISTS "${dest}/include/sherpa-onnx/c-api/c-api.h")
      file(READ "${pin_stamp}" cached_pin)
      if(cached_pin STREQUAL pin_key)
        message(STATUS "sherpa-onnx macOS pin already present at ${dest}")
        return()
      endif()
    endif()
  endif()

  set(download_url "${base_url}/v${version}/${asset}")
  set(archive_path "${CMAKE_SOURCE_DIR}/.deps/${asset}")

  file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/.deps")

  if(NOT EXISTS "${archive_path}")
    message(STATUS "Downloading sherpa-onnx ${version} (${platform_label}): ${download_url}")
    if(expected_hash AND NOT expected_hash STREQUAL "")
      file(DOWNLOAD "${download_url}" "${archive_path}" STATUS download_status EXPECTED_HASH SHA256=${expected_hash})
    else()
      file(DOWNLOAD "${download_url}" "${archive_path}" STATUS download_status)
    endif()
    list(GET download_status 0 download_code)
    list(GET download_status 1 download_msg)
    if(NOT download_code EQUAL 0)
      file(REMOVE "${archive_path}")
      message(FATAL_ERROR "Failed to download sherpa-onnx: ${download_msg}")
    endif()
  endif()

  if(platform_label STREQUAL "macos")
    file(SHA256 "${archive_path}" archive_hash)
    if(NOT archive_hash STREQUAL expected_hash)
      message(FATAL_ERROR "SHA256 mismatch for ${archive_path}; remove the cached archive and reconfigure")
    endif()
  endif()

  # Extract into a staging dir then flatten one level, because the upstream
  # archive has a single top-level directory like "sherpa-onnx-v1.12.39-...".
  set(staging "${CMAKE_SOURCE_DIR}/.deps/_sherpa_staging")
  file(REMOVE_RECURSE "${staging}")
  file(MAKE_DIRECTORY "${staging}")
  file(ARCHIVE_EXTRACT INPUT "${archive_path}" DESTINATION "${staging}")

  file(GLOB staging_children RELATIVE "${staging}" "${staging}/*")
  list(LENGTH staging_children child_count)
  if(child_count EQUAL 1)
    list(GET staging_children 0 top_dir)
    set(extracted_root "${staging}/${top_dir}")
  else()
    set(extracted_root "${staging}")
  endif()

  file(REMOVE_RECURSE "${dest}")
  file(MAKE_DIRECTORY "${dest}")

  # We only need include/ and lib/. Copy them verbatim.
  if(EXISTS "${extracted_root}/include")
    file(COPY "${extracted_root}/include" DESTINATION "${dest}")
  endif()
  if(EXISTS "${extracted_root}/lib")
    file(COPY "${extracted_root}/lib" DESTINATION "${dest}")
  endif()

  file(REMOVE_RECURSE "${staging}")
  if(platform_label STREQUAL "macos")
    if(
      NOT EXISTS "${dest}/include/sherpa-onnx/c-api/c-api.h"
      OR NOT EXISTS "${dest}/lib/libsherpa-onnx-c-api.dylib"
      OR NOT EXISTS "${dest}/lib/libsherpa-onnx-cxx-api.dylib"
      OR NOT EXISTS "${dest}/lib/libonnxruntime.dylib"
    )
      message(FATAL_ERROR "Incomplete sherpa-onnx macOS archive: ${archive_path}")
    endif()
    file(WRITE "${pin_stamp}" "${pin_key}")
  endif()
  message(STATUS "sherpa-onnx ${version} ready at ${dest}")
endfunction()
