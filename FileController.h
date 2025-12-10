#pragma once

#include <QObject>
#include "PropertyMacros.h"

class AppSettings;
class BudgetData;
class CategoryController;
class NavigationController;

class FileController : public QObject {
  Q_OBJECT

  // Current file path (macro-generated)
  PROPERTY_RW(QString, currentFilePath, {})

  // Error message for file operations (empty if no error)
  PROPERTY_RW(QString, errorMessage, {})

  // Read-only property for unsaved changes
  PROPERTY_RO(bool, hasUnsavedChanges)

public:
  explicit FileController(QObject *parent = nullptr);

  // Set references to other controllers
  void setAppSettings(AppSettings *settings);
  void setBudgetData(BudgetData *budgetData);
  void setCategoryController(CategoryController *categoryController);
  void setNavigationController(NavigationController *navController);

  // File operations
  Q_INVOKABLE bool loadFromYaml(const QString &filePath);
  Q_INVOKABLE bool saveToYaml(const QString &filePath);
  Q_INVOKABLE bool importFromCsv(const QString &filePath,
                                 const QString &accountName = QString(),
                                 bool useCategories = false);

  // Clear all data
  Q_INVOKABLE void clear();

signals:
  void dataLoaded();      // Emitted after any data load (YAML or CSV import)
  void yamlFileLoaded();  // Emitted only after YAML file load (for UI state restore)
  void dataSaved();

  // Navigation state signals for file load/save coordination
  void navigationStateLoaded(int tabIndex, int budgetYear, int budgetMonth,
                             int accountIndex, int categoryIndex, int operationIndex);

private:
  AppSettings *_appSettings = nullptr;
  BudgetData *_budgetData = nullptr;
  CategoryController *_categoryController = nullptr;
  NavigationController *_navController = nullptr;
};
