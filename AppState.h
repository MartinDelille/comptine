#pragma once

#include <QObject>
#include <QQmlEngine>

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

private:
  // Declaration order matters - dependencies must come first
  AppSettings _settings;
  BudgetData _data;
  CategoryController _categories;
  RuleController _rules;
  NavigationController _navigation;
  ClipboardController _clipboard;  // Depends on BudgetData::operationModel()
  FileController _file;            // Depends on all 4 controllers
  UpdateController _update;        // Depends on AppSettings
};
