#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QUndoStack>

#include "AppSettings.h"
#include "BudgetData.h"
#include "CategoryController.h"
#include "FileController.h"
#include "RuleController.h"
#include "UndoCommands.h"
#include "UpdateController.h"
#include "Version.h"

class AppState : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  PROPERTY_CONSTANT(QString, appVersion, APP_VERSION_FULL)
  PROPERTY_CONSTANT(QString, appCommitHash, APP_COMMIT_HASH)

  Q_PROPERTY(AppSettings* settings READ settings CONSTANT)
  Q_PROPERTY(BudgetData* budgetData READ budgetData CONSTANT)
  Q_PROPERTY(CategoryController* categories READ categories CONSTANT)
  Q_PROPERTY(FileController* file READ file CONSTANT)
  Q_PROPERTY(RuleController* rules READ rules CONSTANT)
  Q_PROPERTY(UpdateController* update READ update CONSTANT)
  Q_PROPERTY(UndoStack* undoStack READ undoStack CONSTANT)

public:
  explicit AppState(QObject* parent = nullptr);

  AppSettings* settings() { return &_settings; }
  BudgetData* budgetData() { return &_budgetData; }
  CategoryController* categories() { return &_categories; }
  FileController* file() { return &_file; }
  RuleController* rules() { return &_rules; }
  UpdateController* update() { return &_update; }
  UndoStack* undoStack() { return &_undoStack; }

private:
  UndoStack _undoStack;
  AppSettings _settings;
  BudgetData _budgetData;
  CategoryController _categories;
  RuleController _rules;
  FileController _file;
  UpdateController _update;
};
