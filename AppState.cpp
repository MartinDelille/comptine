#include "AppState.h"

AppState::AppState(QObject* parent) :
    QObject(parent),
    _data(_undoStack),
    _categories(_data, _undoStack),
    _rules(_data, _undoStack),
    _navigation(_data, _categories),
    _clipboard(*_data.operationModel()),
    _file(_settings, _data, _categories, _navigation, _rules, _undoStack),
    _update(_settings) {
  // Connect controllers together (circular dependencies require setters)
  _data.setNavigationController(&_navigation);
  _data.setCategoryController(&_categories);

  // Connect navigation state loading signal from FileController
  connect(&_file, &FileController::navigationStateLoaded,
          &_navigation, &NavigationController::onNavigationStateLoaded);
}
