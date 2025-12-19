#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QUndoStack>

#include "AppSettings.h"
#include "BudgetData.h"
#include "CategoryController.h"
#include "ClipboardController.h"
#include "FileController.h"
#include "NavigationController.h"
#include "RuleController.h"
#include "UpdateController.h"

class AppState : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(AppSettings* settings READ settings CONSTANT)
  Q_PROPERTY(BudgetData* data READ data CONSTANT)
  Q_PROPERTY(CategoryController* categories READ categories CONSTANT)
  Q_PROPERTY(ClipboardController* clipboard READ clipboard CONSTANT)
  Q_PROPERTY(NavigationController* navigation READ navigation CONSTANT)
  Q_PROPERTY(FileController* file READ file CONSTANT)
  Q_PROPERTY(RuleController* rules READ rules CONSTANT)
  Q_PROPERTY(UpdateController* update READ update CONSTANT)
  Q_PROPERTY(QUndoStack* undoStack READ undoStack CONSTANT)

public:
  explicit AppState(QObject* parent = nullptr);

  AppSettings* settings() { return &_settings; }
  BudgetData* data() { return &_data; }
  CategoryController* categories() { return &_categories; }
  ClipboardController* clipboard() { return &_clipboard; }
  NavigationController* navigation() { return &_navigation; }
  FileController* file() { return &_file; }
  RuleController* rules() { return &_rules; }
  UpdateController* update() { return &_update; }

public:
  QUndoStack* undoStack() { return &_undoStack; }

private:
  QUndoStack _undoStack;
  AppSettings _settings;
  BudgetData _data;
  NavigationController _navigation;
  CategoryController _categories;
  RuleController _rules;
  ClipboardController _clipboard;
  FileController _file;
  UpdateController _update;
};
