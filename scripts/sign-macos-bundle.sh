#!/usr/bin/env bash

set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "Usage: $0 APP_BUNDLE [EXPECTED_VERSION]" >&2
  exit 2
fi

app_bundle=$1
expected_version=${2:-}

if [[ ! -d "$app_bundle" || "$app_bundle" != *.app ]]; then
  echo "Not a macOS application bundle: $app_bundle" >&2
  exit 2
fi

app_executable="$app_bundle/Contents/MacOS/$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$app_bundle/Contents/Info.plist")"

if [[ ! -f "$app_executable" ]]; then
  echo "Application executable does not exist: $app_executable" >&2
  exit 1
fi

if [[ -n "$expected_version" ]]; then
  actual_version=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$app_bundle/Contents/Info.plist")
  if [[ "$actual_version" != "$expected_version" ]]; then
    echo "Bundle version mismatch: expected $expected_version, got $actual_version" >&2
    exit 1
  fi
fi

mach_o_files() {
  find "$app_bundle/Contents" -type f -print | sort -r | while IFS= read -r path; do
    if file "$path" | grep -q 'Mach-O'; then
      printf '%s\n' "$path"
    fi
  done
}

while IFS= read -r path; do
  # macdeployqt can leave Conan's build-machine RPATHs in deployed binaries.
  # Remove only non-system absolute RPATHs; @rpath and system paths are retained.
  while IFS= read -r rpath; do
    case "$rpath" in
      /Users/*|/private/*|/tmp/*)
        install_name_tool -delete_rpath "$rpath" "$path"
        ;;
    esac
  done < <(otool -l "$path" | awk '
    /LC_RPATH/ { in_rpath=1; next }
    in_rpath && /path / { print $2; in_rpath=0 }
  ')

  # Re-sign nested code first, then sign the app bundle below.
  codesign --force --sign - --timestamp=none "$path"
done < <(mach_o_files)

codesign --force --sign - --timestamp=none "$app_bundle"

echo "Ad-hoc signed macOS bundle: $app_bundle"
