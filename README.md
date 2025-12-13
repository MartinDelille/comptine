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

### macOS

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

## Creating Installers

### macOS (DMG)

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
