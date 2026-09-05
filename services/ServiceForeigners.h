#pragma once

#include <qqmlintegration.h>
#include <QJSEngine>
#include <QQmlEngine>
#include <QUndoStack>

#include "AppSettings.h"
#include "AppState.h"
#include "BudgetData.h"
#include "CategoryController.h"
#include "FileController.h"
#include "RuleController.h"
#include "UpdateController.h"
#include "utils/ForeignBase.h"

struct AppSettingsForeign : ForeignBase<AppSettings> {
  Q_GADGET
  QML_FOREIGN(AppSettings)
  QML_SINGLETON
  QML_NAMED_ELEMENT(AppSettings)
};

struct AppStateForeign : ForeignBase<AppState> {
  Q_GADGET
  QML_FOREIGN(AppState)
  QML_SINGLETON
  QML_NAMED_ELEMENT(AppState)
};

struct BudgetDataForeign : ForeignBase<BudgetData> {
  Q_GADGET
  QML_FOREIGN(BudgetData)
  QML_SINGLETON
  QML_NAMED_ELEMENT(BudgetData)
};

struct CategoryControllerForeign : ForeignBase<CategoryController> {
  Q_GADGET
  QML_FOREIGN(CategoryController)
  QML_SINGLETON
  QML_NAMED_ELEMENT(CategoryController)
};

struct FileControllerForeign : ForeignBase<FileController> {
  Q_GADGET
  QML_FOREIGN(FileController)
  QML_SINGLETON
  QML_NAMED_ELEMENT(FileController)
};

struct RuleControllerForeign : ForeignBase<RuleController> {
  Q_GADGET
  QML_FOREIGN(RuleController)
  QML_SINGLETON
  QML_NAMED_ELEMENT(RuleController)
};

struct UpdateControllerForeign : ForeignBase<UpdateController> {
  Q_GADGET
  QML_FOREIGN(UpdateController)
  QML_SINGLETON
  QML_NAMED_ELEMENT(UpdateController)
};

struct UndoStackForeign : ForeignBase<QUndoStack> {
  Q_GADGET
  QML_FOREIGN(QUndoStack)
  QML_SINGLETON
  QML_NAMED_ELEMENT(UndoStack)
};
