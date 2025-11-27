# Comptine - Agent Guidelines

## Build Commands

- The project use Qt 6.10.1 installed here: ~/Qt/6.10.1/macos
- **Configure**: `qt-cmake -B build/agent -S .`
- **Build**: `qt-cmake --build build/agent`
- **Run**: `./build/agent/appComptine.app/Contents/MacOS/appComptine example.csv`
- **Clean**: `rm -rf build/agent`

## Project Structure

- Qt6 (6.10.1) QML application with C++ backend
- CMake build system using `qt_add_executable` and `qt_add_qml_module`
- Main files: `main.cpp`, `Transaction.{h,cpp}`, `TransactionModel.{h,cpp}`, `Main.qml`

## Code Style

### C++ (Qt Style)

- **Includes**: Qt headers first (`<QObject>`, `<QString>`), then local headers (`"Transaction.h"`)
- **Naming**: Classes `PascalCase`, methods `camelCase`, private members `m_camelCase`
- **Qt Objects**: Inherit from `QObject`, use `Q_OBJECT` macro, explicit constructors with `QObject *parent = nullptr`
- **Properties**: Use `Q_PROPERTY` for QML binding, `Q_INVOKABLE` for callable methods
- **Memory**: Use raw pointers for Qt parent-child ownership (parent deletes children automatically)
- **Formatting**: 2-space indentation, `{` on same line for methods, `const` methods where applicable

### QML

- **Imports**: QtQuick modules first, grouped logically
- **IDs**: `camelCase` (e.g., `listView`, `fileDialog`)
- **Properties**: Declare `required property` for delegate bindings
- **Strings**: Use `qsTr()` for translatable text
- **Formatting**: 4-space indentation, prefer named properties over property bindings where possible
