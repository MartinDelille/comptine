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
cmake --preset=conan-release
cmake --build --preset=conan-release
macdeployqt build/Release/app/Comptine.app -qmldir=.

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

### Signed updates

Release builds publish a signed update manifest. The application verifies each
release asset with Ed25519 before offering it for installation.

Generate the release key pair once on a secure machine:

```bash
openssl genpkey -algorithm ED25519 -out update-private.pem
openssl pkey -in update-private.pem -pubout -outform DER -out update-public.der
python3 -c 'import base64; print(base64.b64encode(open("update-public.der", "rb").read()[-32:]).decode())'
```

The command prints the raw 32-byte public key as base64. Replace the
`publicKey` value in `services/UpdateController.cpp` with that value. Do not
commit either key file or the private key.

Add the complete contents of `update-private.pem` to the GitHub Actions
repository secret named `COMPTINE_UPDATE_PRIVATE_KEY` under Settings > Secrets
and variables > Actions. The release workflow uses this secret to sign the
manifest entries for the macOS, Windows, and Linux artifacts.

Keep the private key in a password manager or another protected secret store.
If it is compromised, generate a new pair, update the embedded public key, and
ship a new application version before publishing further automatic updates.

### Testing updates locally

The `development` CMake preset enables an update-test override. Release builds
do not include it. To test with a local or fake repository, generate a separate
Ed25519 key pair and expose a signed manifest and installer asset from an HTTP
server:

```bash
openssl genpkey -algorithm ED25519 -out test-update-private.pem
openssl pkey -in test-update-private.pem -pubout -outform DER -out test-update-public.der
python3 -c 'import base64; print(base64.b64encode(open("test-update-public.der", "rb").read()[-32:]).decode())'
```

Generate the manifest with `scripts/create-update-manifest.py`, using a version
newer than the running application and an asset URL reachable by the application.
Then launch the development build with the full manifest URL and the base64
public key:

```bash
COMPTINE_UPDATE_MANIFEST_URL=http://127.0.0.1:8000/Comptine-update.json \
COMPTINE_UPDATE_PUBLIC_KEY='<base64 test public key>' \
./build/Debug/app/Comptine.app/Contents/MacOS/Comptine
```

The signed manifest still controls the asset URL, platform, architecture, and
SHA-256 digest. Invalid signatures and modified assets must continue to fail.
Never use the production private key for local testing.

## Website

The project website is in the `docs/` folder and uses Jekyll.

### Serve locally

```bash
cd docs
bundle install  # First time only
bundle exec jekyll serve --livereload
```

Then open http://localhost:4000/comptine

baba
