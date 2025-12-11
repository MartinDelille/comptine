#include "AppState.h"
#include "AppSettings.h"
#include "BudgetData.h"
#include "CategoryController.h"
#include "ClipboardController.h"
#include "FileController.h"
#include "NavigationController.h"
#include "UpdateController.h"

AppState::AppState(QObject *parent) :
    QObject(parent),
    _settings(new AppSettings(this)),
    _data(new BudgetData(this)),
    _categories(new CategoryController(this)),
    _clipboard(new ClipboardController(this)),
    _navigation(new NavigationController(this)),
    _file(new FileController(this)),
    _update(new UpdateController(this)) {
  // Connect controllers together
  _navigation->setBudgetData(_data);
  _navigation->setCategoryController(_categories);
  _data->setNavigationController(_navigation);
  _categories->setBudgetData(_data);
  _categories->setUndoStack(_data->undoStack());
  _clipboard->setOperationModel(_data->operationModel());
  _file->setAppSettings(_settings);
  _file->setBudgetData(_data);
  _file->setNavigationController(_navigation);
  _file->setCategoryController(_categories);
  _update->setAppSettings(_settings);

  // Connect navigation state loading signal from FileController
  connect(_file, &FileController::navigationStateLoaded,
          _navigation, &NavigationController::onNavigationStateLoaded);
}
