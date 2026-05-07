# CaptionFlow Rename Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rename the project's public brand and technical slug from `obs-ai-caption` to `CaptionFlow` / `captionflow` without resubmitting to the OBS resource directory.

**Architecture:** This is a scoped rename across metadata, docs, helper scripts, user-visible strings, and test target names. SDK/dependency references to OBS remain when they describe OBS Studio APIs or install locations rather than project branding.

**Tech Stack:** CMake, C++17, OBS plugin template conventions, GitHub Actions, shell scripts, Markdown docs.

---

## File Structure

- `buildspec.json` owns plugin build metadata: slug, display name, author, website, macOS bundle id.
- `CMakeLists.txt` and `tests/CMakeLists.txt` own offline test executable and CTest target names.
- `src/plugin-main.cpp`, `src/model-downloader.cpp`, and `data/locale/en-US.ini` own user-visible plugin names and author/module strings.
- `README.md`, `CONTRIBUTING.md`, `CHANGELOG.md`, and `docs/*.md` own public documentation and submission/signing copy.
- `scripts/unquarantine-macos.sh`, `scripts/deploy-macos.sh`, and `.github/workflows/push.yaml` own release/package helper paths and CI artifact names.
- `scripts/obs-forum-submit.mjs` is OBS-resource submission automation and should be retired or made non-submitting so it does not encourage immediate resubmission.

## Task 1: Update build metadata and test target names

**Files:**
- Modify: `buildspec.json`
- Modify: `CMakeLists.txt:144-159`
- Modify: `tests/CMakeLists.txt:2-49`

- [ ] **Step 1: Update buildspec metadata**

Change `buildspec.json` so these fields read exactly:

```json
{
  "platformConfig": {
    "macos": {
      "bundleId": "io.github.xwhqsj.captionflow"
    }
  },
  "name": "captionflow",
  "displayName": "CaptionFlow",
  "version": "0.1.0",
  "author": "CaptionFlow contributors",
  "website": "https://github.com/XWHQSJ/obs-ai-caption",
  "email": "xwhqsj@gmail.com"
}
```

Keep the existing `dependencies` object unchanged. The website stays on the current repository URL until the repository is renamed separately.

- [ ] **Step 2: Rename root CMake test executable references**

In `CMakeLists.txt`, change the test target from `obs-ai-caption-tests` to `captionflow-tests` in all four places:

```cmake
  add_executable(captionflow-tests
    ${_CORE_SOURCES}
    tests/test-main.cpp
    tests/subtitle-manager-test.cpp
    tests/mute-word-list-test.cpp
    tests/audio-ring-buffer-test.cpp
    tests/audio-analyzer-test.cpp
    tests/audio-delay-buffer-test.cpp
    tests/model-finder-test.cpp
  )
  target_include_directories(captionflow-tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests)
  if(NOT WIN32)
    target_link_libraries(captionflow-tests PRIVATE pthread)
  endif()
  add_test(NAME captionflow-tests COMMAND captionflow-tests)
```

- [ ] **Step 3: Rename standalone test CMake project and executable**

In `tests/CMakeLists.txt`, replace the project and test target names with `captionflow-tests`:

```cmake
project(captionflow-tests LANGUAGES CXX)
```

```cmake
add_executable(captionflow-tests
  ${_CORE_SOURCES}
  test-main.cpp
  subtitle-manager-test.cpp
  mute-word-list-test.cpp
  audio-ring-buffer-test.cpp
  audio-analyzer-test.cpp
  audio-delay-buffer-test.cpp
  model-finder-test.cpp
)

target_include_directories(captionflow-tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(MSVC)
  target_compile_options(captionflow-tests PRIVATE /W4 /utf-8 /permissive-)
  target_compile_definitions(captionflow-tests PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
  target_compile_options(captionflow-tests PRIVATE -Wall -Wextra -Wpedantic)
  target_link_libraries(captionflow-tests PRIVATE pthread)
endif()

add_test(NAME captionflow-tests COMMAND captionflow-tests)
```

- [ ] **Step 4: Run metadata grep**

Run:

```bash
grep -RIn --exclude-dir=.git --exclude-dir=node_modules --exclude-dir=build --exclude-dir=build_macos --exclude-dir=build_x64 --exclude-dir=.deps --exclude-dir=deps "com\.obsproject\.obs-ai-caption\|obs-ai-caption-tests\|project(obs-ai-caption-tests" .
```

Expected: no matches.

- [ ] **Step 5: Commit metadata changes**

Run:

```bash
git add buildspec.json CMakeLists.txt tests/CMakeLists.txt
git commit -m "refactor: rename plugin metadata to CaptionFlow"
```

## Task 2: Update user-visible source strings

**Files:**
- Modify: `src/plugin-main.cpp:1-31`
- Modify: `src/model-downloader.cpp:115`
- Modify: `data/locale/en-US.ini:1`

- [ ] **Step 1: Update plugin module header and author**

In `src/plugin-main.cpp`, change the header and author to:

```cpp
/*
CaptionFlow
Copyright (C) 2026 CaptionFlow contributors

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/
```

Change the author macro to:

```cpp
OBS_MODULE_AUTHOR("CaptionFlow contributors")
```

Change the module description to:

```cpp
return "On-device real-time captions using sherpa-onnx streaming ASR.";
```

- [ ] **Step 2: Update model downloader window title**

In `src/model-downloader.cpp`, change the window title to:

```cpp
setWindowTitle(tr("CaptionFlow — Download ASR model"));
```

- [ ] **Step 3: Update filter display name**

In `data/locale/en-US.ini`, change the first line to:

```ini
AICaptions="CaptionFlow (Streaming ASR)"
```

Do not rename the localization key in this task. It is internal and changing it would require touching `src/caption-filter.cpp` for no user-facing benefit.

- [ ] **Step 4: Run source-string grep**

Run:

```bash
grep -RIn --exclude-dir=.git "obs-ai-caption contributors\|AI Captions — Download\|AI Captions (Streaming ASR)" src data
```

Expected: no matches.

- [ ] **Step 5: Commit source string changes**

Run:

```bash
git add src/plugin-main.cpp src/model-downloader.cpp data/locale/en-US.ini
git commit -m "refactor: update CaptionFlow user-facing strings"
```

## Task 3: Update release scripts and CI artifact naming

**Files:**
- Modify: `scripts/unquarantine-macos.sh`
- Modify: `scripts/deploy-macos.sh`
- Modify: `.github/workflows/push.yaml:119-135`

- [ ] **Step 1: Update unquarantine helper paths and output**

In `scripts/unquarantine-macos.sh`, use `captionflow.plugin` paths and product text:

```bash
PLUGIN_CANDIDATES=(
  "$HOME/Library/Application Support/obs-studio/plugins/captionflow.plugin"
  "/Library/Application Support/obs-studio/plugins/captionflow.plugin"
)

echo "Looking for captionflow.plugin..."
```

Change the final success message to:

```bash
echo "Done. Relaunch OBS Studio; the CaptionFlow filter should now appear."
```

If the script contains a curl example pointing at the current repository, leave the URL unchanged until the repository is renamed separately.

- [ ] **Step 2: Update local deploy helper**

In `scripts/deploy-macos.sh`, update plugin bundle and data paths:

```bash
# Deploys the built captionflow.plugin into a locally built OBS.app.
```

```bash
PLUGIN_BUNDLE="${PLUGIN_DIR}/captionflow.plugin"
DATA_DIR="${OBS_APP}/Contents/Resources/data/obs-plugins/captionflow"
rm -rf "${PLUGINS_DIR}/captionflow.plugin"
codesign --force --deep --sign - "${PLUGINS_DIR}/captionflow.plugin"
echo "Deployed to ${PLUGINS_DIR}/captionflow.plugin"
```

Keep `PLUGIN_DIR="/Users/bytedance/Workspace/obs-ai-caption/build-plugin-macos"` unchanged because it is a local checkout path, not product branding.

- [ ] **Step 3: Update SignPath CI slugs and artifact path**

In `.github/workflows/push.yaml`, change the SignPath fields to:

```yaml
          project-slug: captionflow
          signing-policy-slug: release-signing
          artifact-configuration-slug: windows-zip
          github-artifact-id: ${{ github.workspace }}/captionflow-${{ needs.build-project.outputs.pluginName && steps.check.outputs.version }}-windows-x64.zip
```

- [ ] **Step 4: Run script grep**

Run:

```bash
grep -RIn --exclude-dir=.git "obs-ai-caption.plugin\|project-slug: obs-ai-caption\|github-artifact-id: .*obs-ai-caption" scripts .github/workflows
```

Expected: no matches.

- [ ] **Step 5: Commit script and CI changes**

Run:

```bash
git add scripts/unquarantine-macos.sh scripts/deploy-macos.sh .github/workflows/push.yaml
git commit -m "ci: rename release artifacts to CaptionFlow"
```

## Task 4: Update README and contributor docs

**Files:**
- Modify: `README.md`
- Modify: `CONTRIBUTING.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Update README primary branding**

In `README.md`, make these replacements:

```markdown
# 🎙️ CaptionFlow

> **On-device, real-time captions for OBS Studio — no cloud, no API keys, no data leaves your machine.**
```

Change the demo alt text to:

```html
<img src="docs/screenshots/hero.gif" alt="CaptionFlow demo" width="720">
```

Change the comparison table header to:

```markdown
| Existing caption plugins | CaptionFlow |
```

Change the cache path line to:

```markdown
- Cached under `~/…/obs-studio/plugin_config/captionflow/`
```

Change release artifact examples to:

```markdown
   - Windows: `captionflow-<version>-windows-x64.zip`
   - macOS:   `captionflow-<version>-macos-universal.pkg`
```

Change the first-use diagram label and step to:

```text
  │  Audio Source │   │  CaptionFlow    │   │ Text (GDI+)    │
```

```markdown
1. Right-click an audio source → **Filters → + → CaptionFlow**
```

Keep current GitHub URLs unchanged in this task. They still point to the current repository.

- [ ] **Step 2: Remove OBS badge as a brand badge**

In the README badge block, delete this line:

```html
  <a href="https://obsproject.com/"><img alt="OBS" src="https://img.shields.io/badge/OBS-31.0%2B-green"></a>
```

The minimum OBS Studio version remains documented in install/build text where needed.

- [ ] **Step 3: Update CONTRIBUTING heading**

Change the first line of `CONTRIBUTING.md` to:

```markdown
# Contributing to CaptionFlow
```

Leave technical references to OBS plumbing where they describe source files or APIs.

- [ ] **Step 4: Update CHANGELOG primary name if present**

If `CHANGELOG.md` contains release headings or prose using `obs-ai-caption` as the primary project name, replace those with `CaptionFlow`. Do not rewrite historical technical dependency entries such as `OBS / Qt6 / sherpa-onnx dependency fetch`.

- [ ] **Step 5: Run docs grep for README/contributor primary name**

Run:

```bash
grep -RIn --exclude-dir=.git "# 🎙️ obs-ai-caption\|Existing caption plugins | obs-ai-caption\|AI Captions    │\|Filters → + → AI Captions\|obs-ai-caption demo\|obs-studio/plugin_config/obs-ai-caption\|obs-ai-caption-<version>\|Contributing to obs-ai-caption" README.md CONTRIBUTING.md CHANGELOG.md
```

Expected: no matches.

- [ ] **Step 6: Commit README and contributor docs**

Run:

```bash
git add README.md CONTRIBUTING.md CHANGELOG.md
git commit -m "docs: rebrand project as CaptionFlow"
```

## Task 5: Update submission and signing docs

**Files:**
- Modify: `docs/submission.md`
- Modify: `docs/signpath-setup.md`
- Modify: `docs/forum-description.md`
- Modify: `scripts/obs-forum-submit.mjs`

- [ ] **Step 1: Update submission kit title and Discord copy**

In `docs/submission.md`, change the title to:

```markdown
# Submission kits for CaptionFlow
```

Change Discord-style announcement text from old branding to:

```markdown
Hey folks — just shipped **CaptionFlow** v0.1.0, an MIT-licensed,
on-device streaming ASR captioning plugin for OBS Studio.
```

Keep current GitHub URLs unchanged until the repository is renamed separately.

- [ ] **Step 2: Update SignPath setup metadata**

In `docs/signpath-setup.md`, update the metadata table values to:

```markdown
| Project Name | `CaptionFlow` |
| Repository URL | `https://github.com/XWHQSJ/obs-ai-caption` |
| Homepage URL | `https://github.com/XWHQSJ/obs-ai-caption` |
| Download URL | `https://github.com/XWHQSJ/obs-ai-caption/releases/latest` |
| Tagline | `Free, on-device streaming captions for OBS Studio` |
| Description | `Real-time streaming ASR captions for OBS Studio powered by sherpa-onnx, with an adaptive-beep sensitive-word mute filter. Runs fully on-device; no cloud, no API keys. MIT licensed, bilingual Chinese + English.` |
```

Change the SignPath badge link slug from `/projects/obs-ai-caption` to `/projects/captionflow`.

- [ ] **Step 3: Rewrite forum description for archival use**

In `docs/forum-description.md`, change the heading and comparison header to:

```markdown
# CaptionFlow — on-device streaming ASR captions for OBS Studio
```

```markdown
| Traditional caption plugins | **CaptionFlow** |
```

Change first-use step to:

```markdown
1. Right-click an audio source → **Filters** → **+** → **CaptionFlow**.
```

Change package example to:

```bash
gh attestation verify captionflow-0.1.0-macos-universal.pkg \
  --repo XWHQSJ/obs-ai-caption
```

- [ ] **Step 4: Disable OBS forum auto-submit helper by default**

At the top of `scripts/obs-forum-submit.mjs`, after imports, add:

```js
if (process.env.CAPTIONFLOW_ENABLE_OBS_FORUM_SUBMIT !== '1') {
  console.error('OBS forum submission automation is disabled. Distribution remains GitHub Releases unless policy approval is confirmed.');
  console.error('Set CAPTIONFLOW_ENABLE_OBS_FORUM_SUBMIT=1 only after a separate approval decision.');
  process.exit(1);
}
```

Update `SUBMISSION_DATA` to CaptionFlow wording:

```js
const SUBMISSION_DATA = {
  title: 'CaptionFlow — on-device streaming ASR captions',
  tagline: 'Free, offline, bilingual (EN + 中文) captions powered by sherpa-onnx',
  version: '0.1.0',
  external_url: 'https://github.com/XWHQSJ/obs-ai-caption/releases/tag/0.1.0',
  description: forumDesc,
};
```

- [ ] **Step 5: Run submission docs grep**

Run:

```bash
grep -RIn --exclude-dir=.git "Submission kits for AI Captions\|\*\*obs-ai-caption\*\*\|Project Name | `obs-ai-caption`\|projects/obs-ai-caption\|# AI Captions\|Traditional caption plugins | \*\*AI Captions\*\*\|Filters.*AI Captions\|gh attestation verify obs-ai-caption" docs scripts/obs-forum-submit.mjs
```

Expected: no matches.

- [ ] **Step 6: Commit submission docs changes**

Run:

```bash
git add docs/submission.md docs/signpath-setup.md docs/forum-description.md scripts/obs-forum-submit.mjs
git commit -m "docs: update CaptionFlow distribution materials"
```

## Task 6: Final repository-wide validation and review

**Files:**
- Inspect only unless issues are found.

- [ ] **Step 1: Run repository-wide old-brand grep**

Run:

```bash
grep -RIn --exclude-dir=.git --exclude-dir=node_modules --exclude-dir=build --exclude-dir=build_macos --exclude-dir=build_x64 --exclude-dir=.deps --exclude-dir=deps "obs-ai-caption\|com\.obsproject\.obs-ai-caption\|AI Captions" .
```

Expected: remaining matches are limited to current GitHub URLs, local checkout paths, historical/spec docs, or intentional internal compatibility references. There should be no active primary product branding that says `obs-ai-caption` or `AI Captions`.

- [ ] **Step 2: Run OBS primary-name grep**

Run:

```bash
grep -RIn --exclude-dir=.git --exclude-dir=node_modules --exclude-dir=build --exclude-dir=build_macos --exclude-dir=build_x64 --exclude-dir=.deps --exclude-dir=deps "OBS CaptionFlow\|OBS AI\|official OBS\|# .*OBS" .
```

Expected: no matches that use OBS as the project's primary name. SDK/dependency text like `OBS::libobs`, `OBS Studio`, and `obs-studio` is allowed.

- [ ] **Step 3: Configure offline tests**

Run:

```bash
cmake -S tests -B build-tests
```

Expected: CMake configure completes and writes build files to `build-tests`. If local compiler/CMake dependencies are missing, record the exact error in the final report and continue to grep validation.

- [ ] **Step 4: Build offline tests**

Run:

```bash
cmake --build build-tests -j
```

Expected: target `captionflow-tests` builds successfully. If it fails due to a local toolchain issue unrelated to the rename, record the exact error.

- [ ] **Step 5: Run offline tests**

Run:

```bash
ctest --test-dir build-tests --output-on-failure
```

Expected: all tests pass. Existing README says this suite should report `45 passed, 0 failed`.

- [ ] **Step 6: Review changed diff**

Run:

```bash
git diff --stat HEAD~5..HEAD
git diff HEAD~5..HEAD -- README.md buildspec.json CMakeLists.txt tests/CMakeLists.txt src/plugin-main.cpp src/model-downloader.cpp data/locale/en-US.ini scripts docs .github/workflows/push.yaml
```

Expected: changes are limited to rename, docs, CI artifact naming, helper scripts, and the disabled OBS forum submission helper.

- [ ] **Step 7: Use required code review agent**

Invoke `code-reviewer` with this prompt:

```text
Review the CaptionFlow rename changes in /Users/bytedance/Workspace/obs-ai-caption. Focus on accidental OBS primary-branding leftovers, broken build/test target names, unsafe script behavior, and whether OBS-resource resubmission remains disabled by default. Report only CRITICAL/HIGH/MEDIUM findings with exact file paths.
```

Fix any CRITICAL or HIGH findings before continuing. Fix MEDIUM findings when they are straightforward and within rename scope.

- [ ] **Step 8: Commit final fixes if any**

If review or validation required changes, commit them:

```bash
git add <changed-files>
git commit -m "fix: complete CaptionFlow rename validation"
```

If no changes are needed, do not create an empty commit.

- [ ] **Step 9: Final report**

Report:

```text
DONE
Renamed public branding to CaptionFlow, kept OBS references descriptive/technical only, disabled OBS forum submission automation by default, and validated with grep plus offline tests.

Commits:
- <commit hashes and subjects>

Validation:
- <command>: <result>
- <command>: <result>

Notes:
- GitHub repository URL still points to XWHQSJ/obs-ai-caption until separately renamed.
- OBS resource directory was not resubmitted.
```
