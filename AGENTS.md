# Comptine - Agent Guidelines

## Build Commands

Use exact commands below to configure, build, run, and clean the project. If you have issues, please report them instead of changing commands.

- The project use Qt 6.10.1 installed here: ~/Qt/6.10.1/macos
- **Configure**: `qt-cmake -B build/agent -S .`
- **Build**: `cmake --build build/agent`
- **Run**: `./build/agent/Comptine.app/Contents/MacOS/Comptine`
- **Clean**: `rm -rf build/agent` (run it only if you need a full clean)

When done, you can run the application with a sample CSV file as shown above to show the progress.

## Project Structure

- Qt6 (6.10.1) QML application with C++ backend
- CMake build system using `qt_add_executable` and `qt_add_qml_module`
- Main files: `main.cpp`, `Operation.{h,cpp}`, `BudgetData.{h,cpp}`, `Main.qml`

## Code Style

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
