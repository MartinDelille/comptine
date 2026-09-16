#!/usr/bin/env bash

set -euo pipefail

coverage_build_dir="${1:-build/Coverage}"
coverage_file="${2:-coverage.info}"

lcov --capture \
  --directory "$coverage_build_dir" \
  --output-file "$coverage_file" \
  --ignore-errors mismatch,inconsistent,unsupported,gcov,format

lcov --remove "$coverage_file" \
  '*/tests/*' \
  '*/build/*' \
  '*/Qt/*' \
  '*/.conan2/*' \
  '/usr/*' \
  '*/Xcode.app/*' \
  --output-file "$coverage_file" \
  --ignore-errors unused,inconsistent,unsupported,gcov,format
