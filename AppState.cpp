#include "AppState.h"

AppState::AppState(QObject* parent) :
    QObject(parent),
    _data(_undoStack),
    _categories(_undoStack),
    _rules(_undoStack),
    _clipboard(*_data.operationModel()),
    _file(_settings, _data, _categories, _navigation, _rules, _undoStack),
    _update(_settings) {
  // Connect controllers together (circular dependencies require setters)
  _navigation.setBudgetData(&_data);
  _navigation.setCategoryController(&_categories);
  _data.setNavigationController(&_navigation);
  _data.setCategoryController(&_categories);
  _categories.setBudgetData(&_data);

  _rules.setBudgetData(&_data);

  // Connect navigation state loading signal from FileController
  connect(&_file, &FileController::navigationStateLoaded,
          &_navigation, &NavigationController::onNavigationStateLoaded);
}
