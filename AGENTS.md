# Comptine - Agent Guidelines

## Build Commands

Use exact commands below to configure, build, run, and clean the project. If you have issues, please report them instead of changing commands.

### MacOS

- The project uses the Qt version specified in `.qt-version`, installed at: ~/Qt/{version}/macos
- **Install dependencies**: `conan install . --build=missing`
- **Configure**: `qt-cmake --preset=conan-debug`
- **Build**: `cmake --build --preset=conan-debug`
- **Run**: `./build/Comptine.app/Contents/MacOS/Comptine`
- **Clean**: `rm -rf build` (run it only if you need a full clean)

When done, you can run the application as shown above to show the progress.

## Project Structure

- Qt6 QML application with C++ backend (version specified in `.qt-version`)
- CMake build system using `qt_add_executable` and `qt_add_qml_module`
- Main files: `main.cpp`, `Operation.{h,cpp}`, `BudgetData.{h,cpp}`, `Main.qml`

## Code Style

### Architecture and implementation quality

- For implementation requests, use the project’s native framework abstractions and established architecture directly. Avoid temporary or “cheap” representations when the requirements call for a structured, typed, or reusable design.
- Keep domain calculations and data contracts in the appropriate backend/model layer, and let the UI consume them through the framework’s intended view and binding mechanisms.
- Before creating a custom workaround, check the project’s configured framework version and its official documentation for existing facilities that match the required behavior.
- Prefer a clean, extensible implementation on the first pass. Use provisional structures only when the data is genuinely small and static, or when the user explicitly requests a prototype.

### C++ (Qt Style)

- **Includes**: Qt headers first (`<QObject>`, `<QString>`), then local headers (`"Transaction.h"`)
- **Naming**: Classes `PascalCase`, methods `camelCase`, private members `_camelCase`
- **Qt Objects**: Inherit from `QObject`, use `Q_OBJECT` macro, explicit constructors with `QObject *parent = nullptr`
- **Properties**: Use macros from `PropertyMacros.h` instead of manual `Q_PROPERTY` declarations:
  - `PROPERTY_RW(Type, name, default)` - Full read-write from QML and C++
  - `PROPERTY_RO(Type, name)` - Read-only computed property (implement getter in .cpp)
  - `PROPERTY_RW_CUSTOM(Type, name, default)` - Custom getter/setter logic (implement both in .cpp)
- **Memory**: Use raw pointers for Qt parent-child ownership (parent deletes children automatically)
- **Formatting**: 2-space indentation, `{` on same line for methods, `const` methods where applicable

### QML

- **Imports**: QtQuick modules first, grouped logically
- **IDs**: `camelCase` (e.g., `listView`, `fileDialog`)
- **Properties**: Declare `required property` for delegate bindings
- **Strings**: Use `qsTr()` for translatable text
- **Translations**: When adding new `qsTr()` strings, update all translation files in `translations/` with appropriate translations
- **Formatting**: 4-space indentation, prefer named properties over property bindings where possible

### Reusable Components

- **AmountField**: Use `AmountField.qml` for all monetary amount input fields.

## Testing Guidelines

- **Fake Data**: When creating unit tests, always use fake/fictional data instead of real personal information. Replace names, account numbers, references, and other identifiable data with obviously fictional equivalents (e.g., "NICK LARSONO" instead of real names, "9876543XY0012345" instead of real reference numbers).

## Documentation

- **FEATURES.md**: When adding new features, update `FEATURES.md` to document them.

## Undo/Redo

- **Undoable Actions**: When adding new actions that modify data (e.g., adding/removing operations, renaming, importing), make them undoable using `QUndoCommand` subclasses in `UndoCommands.h/.cpp`.
- **When in doubt**: Ask if an action should be undoable. Generally, any action that modifies user data should support undo/redo.
