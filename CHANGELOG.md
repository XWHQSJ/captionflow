# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Fixed
- Windows packages now install the sherpa-onnx and ONNX Runtime DLLs beside the plugin, avoiding missing-runtime load failures (issue #3). Packaging checks reject missing or mismatched runtime files.
- Windows installation instructions now match the packaged `captionflow/bin/64bit` and `captionflow/data` layout under `%ProgramData%/obs-studio/plugins`.

### Changed
- Caption files are written on a background thread. Partial results no longer stall the OBS audio callback or the ASR decode thread on disk, and a newer caption cannot be overwritten by an older one.
- 48 kHz input is lowpass-decimated to 16 kHz before it enters the ASR queue, so the queue holds 4 seconds at the model rate and sherpa-onnx does not resample that stream again.
- Sensitive-word pitch detection runs an 8 kHz autocorrelation with parabolic peak interpolation, instead of a full-rate search on the audio callback.
- The censor beep is read from a sine table instead of calling `sin` for every sample.
- sherpa-onnx model debug logging is off, so loading a model no longer dumps debug output into the OBS log.
- The audio/ASR ring signals the decode thread only when the buffer goes from empty to non-empty.

## [0.2.0] - 2026-05-17

First public release as **CaptionFlow** — rebranded from the earlier
`obs-ai-caption` internal name, packaged for Windows x64 and macOS universal,
ready for the OBS Forum Resources section.

### Added
- GitHub Actions CI for Windows x64 and macOS universal
- On-demand ASR model downloader UI with 3 preset models
- Optional SHA-256 verification of downloaded model archives
  (per-preset opt-in via the new `sha256_hex` field on `ModelPreset`)
- Simplified Chinese locale (`data/locale/zh-CN.ini`) — UI now follows
  the OBS UI language for CN users, matching the README's bilingual promise
- Project restructured to match the official `obs-plugintemplate`
- `buildspec.json` drives OBS / Qt6 / sherpa-onnx dependency fetch

### Changed
- Plugin now loads via `obs_register_source` from C++ `plugin-main.cpp`
- Settings UI gains a **Download Model…** button
- `AsrEngine::Impl` now held via `std::unique_ptr` for exception-safe
  lifetime (no leak if the rest of the ctor throws)
- `obs_module_unload` is now `MODULE_EXPORT`-qualified for consistency
  with `obs_module_description`

### Fixed
- Consumer-side lost-wakeup race in `AudioRingBuffer::wait_for_data`
- Caption file never being written because of duplicate
  `should_emit_file_update` consumption
- ASR stats log de-duplicating against stale counters after engine restart
- Audio filter no longer dereferences NULL when `obs_get_audio()` returns
  NULL during OBS teardown
- Removed the dead `extracted_subdir_` member in `DownloadDialog`
  (`tar --strip-components=1` already covers the layout flatten)

## [0.1.0] - 2026-04-18

Initial development release — Windows-only; not distributed publicly.
