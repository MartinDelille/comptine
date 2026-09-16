# Comptine

La Compta qui Chante !

![Comptine](comptine.svg)

## Build Instructions

### Prerequisites

- Qt 6.8+ (see `.qt-version` for exact version)
- CMake 3.23+
- librsvg (for icon generation from SVG)
- ImageMagick (Windows only, for ICO creation)
- NSIS (Windows only, for installer)

### MacOS

```bash
# Install librsvg
brew install librsvg

# Configure
qt-cmake -B build -S .

# Build
cmake --build build

# Run
./build/app/Comptine.app/Contents/MacOS/Comptine
```

### Windows

```powershell
# Install ImageMagick (for ICO creation) and NSIS
choco install imagemagick nsis -y

# Configure
cmake -B build -S .

# Build
cmake --build build --config Release

# Run
.\build\Release\Comptine.exe
```

### Code coverage

Coverage is generated in GitHub Actions for Linux builds and reported by Codecov on
pull requests. To generate it locally, install `lcov`, install the Debug dependencies,
and use the dedicated coverage preset. Normal Debug builds remain uninstrumented.

```bash
brew install lcov
uv run conan install . --build=missing \
  -pr:h=conan/profiles/macos \
  -pr:b=conan/profiles/macos \
  -s:h build_type=Debug \
  -s:b build_type=Debug
cmake --preset=coverage
cmake --build --preset=coverage
ctest --test-dir build/Coverage --output-on-failure
bash scripts/generate-coverage.sh
genhtml coverage.info \
  --output-directory coverage-html \
  --ignore-errors inconsistent,corrupt,format,category
open coverage-html/index.html
```

The coverage preset uses `build/Coverage`, so it does not affect the regular
`development` Debug build in `build/Debug`. Conan generates `ConanPresets.json`
with the dependency/toolchain presets used by the project presets. The same
workflow is available through `make coverage`.

## Creating Installers

### MacOS (DMG)

```bash
# Build and deploy Qt dependencies
qt-cmake -B build -S .
cmake --build build
macdeployqt build/app/Comptine.app -qmldir=.

# Create DMG installer
cd build && cpack -G DragNDrop
```

The installer will be at `build/Comptine-<version>-MacOS.dmg`

### Windows (NSIS)

```powershell
# Build and deploy Qt dependencies
cmake -B build -S .
cmake --build build --config Release
windeployqt build/Release/Comptine.exe --qmldir .

# Create NSIS installer
cd build
cpack -G NSIS -C Release
```

The installer will be at `build/Comptine-<version>-Windows.exe`

## Website

The project website is in the `docs/` folder and uses Jekyll.

### Serve locally

```bash
cd docs
bundle install  # First time only
bundle exec jekyll serve --livereload
```

Then open http://localhost:4000/comptine
