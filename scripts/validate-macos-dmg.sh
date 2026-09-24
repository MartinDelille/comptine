#!/usr/bin/env bash

set -euo pipefail

if [[ $# -lt 3 || $# -gt 4 ]]; then
  echo "Usage: $0 DMG [EXPECTED_VERSION] EXPECTED_ARCH [DATA_FILE]" >&2
  exit 2
fi

dmg=$1
expected_version=$2
expected_arch=$3
data_file=${4:-}
mountpoint=$(mktemp -d)
app_pid=""

cleanup() {
  set +e
  if [[ -n "$app_pid" ]]; then
    kill "$app_pid" 2>/dev/null || true
    wait "$app_pid" 2>/dev/null || true
  fi
  hdiutil detach "$mountpoint" -quiet 2>/dev/null || true
  rmdir "$mountpoint" 2>/dev/null || true
}

trap cleanup EXIT

if [[ ! -f "$dmg" ]]; then
  echo "DMG not found: $dmg" >&2
  exit 1
fi

hdiutil attach "$dmg" -nobrowse -readonly -mountpoint "$mountpoint"
app_bundle="$mountpoint/Comptine.app"

bash "$(dirname "$0")/validate-macos-bundle.sh" "$app_bundle" "$expected_version"
app_executable="$app_bundle/Contents/MacOS/Comptine"
actual_arch=$(lipo -archs "$app_executable")
if [[ "$actual_arch" != "$expected_arch" ]]; then
  echo "DMG architecture mismatch: expected $expected_arch, got $actual_arch" >&2
  exit 1
fi

if [[ -n "$data_file" ]]; then
  if [[ ! -f "$data_file" ]]; then
    echo "Smoke-test data file not found: $data_file" >&2
    exit 1
  fi

  log_file=$(mktemp)
  trap 'rm -f "$log_file"; cleanup' EXIT
  "$app_executable" "$data_file" >"$log_file" 2>&1 &
  app_pid=$!

  for _ in {1..10}; do
    if ! kill -0 "$app_pid" 2>/dev/null; then
      echo "Packaged app exited during smoke test" >&2
      cat "$log_file" >&2
      exit 1
    fi
    sleep 1
  done

  kill "$app_pid" 2>/dev/null || true
  wait "$app_pid" 2>/dev/null || true
  app_pid=""
  rm -f "$log_file"
  echo "Packaged app launch smoke test passed"
fi

echo "Validated macOS DMG: $dmg"
