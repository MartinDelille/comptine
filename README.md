# Comptine

La Compta qui Chante !

![Comptine](comptine.svg)

## Build Instructions

### Prerequisites

- Qt 6.8+ (see `.qt-version` for exact version)
- CMake 3.16+
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
./build/Comptine.app/Contents/MacOS/Comptine
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
pull requests. To generate it locally, install `lcov`, install dependencies, and use
the debug preset with coverage enabled:

```bash
uv run conan install . --build=missing -pr:h=conan/profiles/linux -pr:b=conan/profiles/linux -s:h build_type=Debug -s:b build_type=Debug
qt-cmake --preset=conan-debug -DCOMPTINE_ENABLE_COVERAGE=ON
cmake --build --preset=conan-debug
ctest --test-dir build/Debug --output-on-failure
lcov --capture --directory build/Debug --output-file coverage.info
lcov --list coverage.info
```

## Creating Installers

### MacOS (DMG)

```bash
# Build and deploy Qt dependencies
qt-cmake -B build -S .
cmake --build build
macdeployqt build/Comptine.app -qmldir=.

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
