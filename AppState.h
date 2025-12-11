#pragma once

#include <QObject>
#include <QQmlEngine>
#include "AppSettings.h"
#include "BudgetData.h"
#include "CategoryController.h"
#include "ClipboardController.h"
#include "FileController.h"
#include "NavigationController.h"
#include "UpdateController.h"

class AppState : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(AppSettings *settings READ settings CONSTANT)
  Q_PROPERTY(BudgetData *data READ data CONSTANT)
  Q_PROPERTY(CategoryController *categories READ categories CONSTANT)
  Q_PROPERTY(ClipboardController *clipboard READ clipboard CONSTANT)
  Q_PROPERTY(NavigationController *navigation READ navigation CONSTANT)
  Q_PROPERTY(FileController *file READ file CONSTANT)
  Q_PROPERTY(UpdateController *update READ update CONSTANT)

public:
  explicit AppState(QObject *parent = nullptr);

  AppSettings *settings() const { return _settings; }
  BudgetData *data() const { return _data; }
  CategoryController *categories() const { return _categories; }
  ClipboardController *clipboard() const { return _clipboard; }
  NavigationController *navigation() const { return _navigation; }
  FileController *file() const { return _file; }
  UpdateController *update() const { return _update; }

private:
  AppSettings *_settings;
  BudgetData *_data;
  CategoryController *_categories;
  ClipboardController *_clipboard;
  NavigationController *_navigation;
  FileController *_file;
  UpdateController *_update;
};
