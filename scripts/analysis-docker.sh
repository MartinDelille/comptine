#!/usr/bin/env bash

set -euo pipefail

readonly host_os="$(uname -s)"

if [[ "${host_os}" != "Darwin" && "${host_os}" != "Linux" ]]; then
  echo "The Docker analysis wrapper supports macOS and Linux." >&2
  exit 1
fi

if [[ "${host_os}" == "Darwin" ]]; then
  if ! command -v colima >/dev/null 2>&1; then
    echo "Colima is required. Install it with: brew install colima" >&2
    exit 1
  fi

  if ! colima status >/dev/null 2>&1; then
    echo "Colima is not running. Start it with: colima start" >&2
    exit 1
  fi
fi

if ! command -v docker >/dev/null 2>&1; then
  echo "Docker CLI is required. Install it with: brew install docker" >&2
  exit 1
fi

readonly image_name="comptine-analysis:$(tr -d '[:space:]' < .qt-version)"
readonly platform="linux/amd64"
readonly workspace_volume="comptine-analysis-workspace"
readonly qt_version="$(tr -d '[:space:]' < .qt-version)"
readonly analysis_image="${COMPTINE_ANALYSIS_IMAGE:-${image_name}}"

if [[ "${COMPTINE_ANALYSIS_SKIP_BUILD:-0}" != "1" ]]; then
  docker build \
    --platform "${platform}" \
    --build-arg "QT_VERSION=${qt_version}" \
    --tag "${analysis_image}" \
    --file docker/analysis.Dockerfile \
    docker
fi

docker volume create "${workspace_volume}" >/dev/null

docker run --rm \
  --platform "${platform}" \
  --volume "${PWD}:/input:ro" \
  --volume "${workspace_volume}:/workspace" \
  --workdir /workspace \
  "${analysis_image}" \
  bash -euxo pipefail -c '
    rm -rf source
    mkdir source
    tar -C /input \
      --exclude=./build \
      --exclude=./.git \
      --exclude=./.venv \
      --exclude=./.venv-* \
      -cf - . | tar -C source -xf -
    cd source
    conan install . \
      --build=missing \
      -pr:h=conan/profiles/linux \
      -pr:b=conan/profiles/linux \
      -s:h build_type=Debug \
      -s:b build_type=Debug
    cmake --preset=analysis \
      -DCMAKE_PREFIX_PATH="${QT_ROOT}" \
      -DCOMPTINE_CLANG_TIDY_EXECUTABLE=/usr/bin/clang-tidy-22 \
      -DCOMPTINE_CLAZY_STANDALONE_EXECUTABLE=/opt/clazy/bin/clazy-standalone \
      -DCOMPTINE_CLAZY_CLANGXX_EXECUTABLE=/usr/lib/llvm-22/bin/clang++
    cmake --build --preset=analysis
    cmake --build --preset=analysis --target clazy-analysis
  '
