# CaptionFlow Rename Design

## Context

The OBS forum rejected the `obs-ai-caption` resource submission and requested that the project remove `OBS` from the primary name. The rejection also stated that OBS does not want its name associated with AI-based projects or plugins. The rename should reduce trademark/IP friction while keeping the project understandable as a plugin for OBS Studio.

## Decision

Rename the public project brand from `obs-ai-caption` to `CaptionFlow`.

Use `CaptionFlow` for the primary product name, release titles, documentation headings, package names, and user-facing copy. Use `captionflow` for lowercase technical identifiers where a slug is needed.

Allowed descriptive wording:

- `CaptionFlow is an on-device real-time captioning plugin for OBS Studio.`
- `Install the plugin into your OBS Studio plugins directory.`
- dependency references such as `obs-studio`, `OBS::libobs`, and `obs-frontend-api`

Avoid primary-name wording that implies affiliation or endorsement:

- `OBS AI Caption`
- `OBS CaptionFlow`
- `obs-ai-caption`
- `official OBS`
- any title or release label that starts with `OBS`

## Scope

Update public and build metadata in this repository:

- README product heading, badges, links, install examples, comparison table, cache paths, and support links
- submission and signing docs under `docs/`
- `buildspec.json` name, display name if needed, author, website, and macOS bundle id
- CMake test target names and test project names that currently use `obs-ai-caption`
- helper scripts that refer to plugin bundle names or install paths
- visible strings in source/data files if they use the old primary name

Do not rename the GitHub repository or remote in this implementation pass. Existing GitHub URLs may temporarily remain as compatibility links until the repository is renamed separately. If copy needs to point to the current repo, it should present the product as `CaptionFlow` and the URL as an implementation detail.

Do not resubmit to the OBS resource directory in this implementation pass. Distribution remains GitHub Releases unless the policy is clarified.

## Compatibility

The technical plugin slug should become `captionflow` for new builds. This may change install paths and config directories. That is acceptable for the next release because the prior OBS forum submission was not approved and the project is still early.

Where user data could be affected, prefer clear release notes over compatibility shims. Do not add migration code unless the current code already has a supported settings migration mechanism.

## Validation

After changes:

1. Search the repository for `obs-ai-caption`, `com.obsproject.obs-ai-caption`, and primary-name uses of `OBS`.
2. Confirm remaining `OBS` references are descriptive or SDK/dependency references, not branding.
3. Run the narrowest available tests for the project, starting with the offline CMake/CTest path if it is configured locally.
4. Run a code review pass after modifications.

## Success Criteria

- Public branding consistently says `CaptionFlow`.
- Lowercase technical identifiers use `captionflow` where practical.
- No primary project name contains `OBS`.
- The repo still builds or the narrow validation command clearly identifies any pre-existing local dependency blocker.
- No OBS resource resubmission happens without a separate approval decision.
