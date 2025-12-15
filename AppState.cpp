#include "AppState.h"

AppState::AppState(QObject* parent) :
    QObject(parent),
    _settings(this),
    _data(this),
    _categories(this),
    _rules(this),
    _navigation(this),
    _clipboard(*_data.operationModel(), this),
    _file(_settings, _data, _categories, _navigation, _rules, this),
    _update(_settings, this) {
  // Connect controllers together (circular dependencies require setters)
  _navigation.setBudgetData(&_data);
  _navigation.setCategoryController(&_categories);
  _data.setNavigationController(&_navigation);
  _data.setCategoryController(&_categories);
  _categories.setBudgetData(&_data);
  _categories.setUndoStack(_data.undoStack());
  _rules.setBudgetData(&_data);
  _rules.setUndoStack(_data.undoStack());

  // Connect navigation state loading signal from FileController
  connect(&_file, &FileController::navigationStateLoaded,
          &_navigation, &NavigationController::onNavigationStateLoaded);
}
