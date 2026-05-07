#!/usr/bin/env bash
# One-line Gatekeeper unquarantine helper for macOS users who install the
# unsigned .pkg / .plugin. Drops the quarantine attribute that prevents OBS
# from loading the plugin silently.
#
# Usage:
#   curl -L https://github.com/XWHQSJ/obs-ai-caption/raw/main/scripts/unquarantine-macos.sh | bash
# …or, after cloning:
#   bash scripts/unquarantine-macos.sh
set -euo pipefail

PLUGIN_CANDIDATES=(
  "$HOME/Library/Application Support/obs-studio/plugins/captionflow.plugin"
  "/Library/Application Support/obs-studio/plugins/captionflow.plugin"
)

echo "Looking for captionflow.plugin..."

found=0
for path in "${PLUGIN_CANDIDATES[@]}"; do
  if [[ -d "$path" ]]; then
    echo "  -> $path"
    if [[ -w "$path" ]]; then
      xattr -cr "$path"
    else
      sudo xattr -cr "$path"
    fi
    found=1
  fi
done

if [[ $found -eq 0 ]]; then
  echo "Plugin bundle not found. Install the .pkg first, then re-run this script." >&2
  exit 1
fi

echo "Done. Relaunch OBS Studio; the CaptionFlow filter should now appear."
