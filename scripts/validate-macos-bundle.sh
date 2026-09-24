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

plist="$app_bundle/Contents/Info.plist"
app_name=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$plist")
app_executable="$app_bundle/Contents/MacOS/$app_name"
actual_version=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$plist")

if [[ -n "$expected_version" && "$actual_version" != "$expected_version" ]]; then
  echo "Bundle version mismatch: expected $expected_version, got $actual_version" >&2
  exit 1
fi

if [[ ! -x "$app_executable" ]]; then
  echo "Application executable is not executable: $app_executable" >&2
  exit 1
fi

codesign --verify --deep --strict --verbose=2 "$app_bundle"

while IFS= read -r path; do
  if ! file "$path" | grep -q 'Mach-O'; then
    continue
  fi

  if otool -l "$path" | grep -E 'path (/Users/|/private/|/tmp/)' >/dev/null; then
    echo "Mach-O file contains a build-machine or temporary RPATH: $path" >&2
    otool -l "$path" >&2
    exit 1
  fi

  if otool -L "$path" | grep -E '(/Users/|/private/|/tmp/)' >/dev/null; then
    echo "Mach-O file contains a build-machine or temporary dependency: $path" >&2
    otool -L "$path" >&2
    exit 1
  fi
done < <(find "$app_bundle/Contents" -type f -print)

echo "Validated macOS bundle version $actual_version ($(lipo -archs "$app_executable"))"
