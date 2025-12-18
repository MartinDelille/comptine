#pragma once

#include <QObject>
#include <QString>
#include <QUndoStack>
#include <QUrl>

#include "PropertyMacros.h"

class AppSettings;
class BudgetData;
class CategoryController;
class NavigationController;
class RuleController;

class FileController : public QObject {
  Q_OBJECT

  // Current file path (macro-generated)
  PROPERTY_RW(QString, currentFilePath, {})

  // Error message for file operations (empty if no error)
  PROPERTY_RW(QString, errorMessage, {})

  // Read-only property for unsaved changes
  PROPERTY_RO(bool, hasUnsavedChanges)

public:
  FileController(AppSettings& appSettings,
                 BudgetData& budgetData,
                 CategoryController& categoryController,
                 NavigationController& navController,
                 RuleController& ruleController,
                 QUndoStack& undoStack);

  // File operations
  Q_INVOKABLE bool loadFromYamlUrl(const QUrl& fileUrl);
  Q_INVOKABLE bool loadFromYamlFile(const QString& filePath);
  Q_INVOKABLE bool saveToYamlUrl(const QUrl& fileUrl);
  Q_INVOKABLE bool saveToYamlFile(const QString& filePath);
  Q_INVOKABLE bool importFromCsv(const QUrl& fileUrl,
                                 const QString& accountName = QString(),
                                 bool useCategories = false);

  // Load initial file from command line arguments or most recent file
  void loadInitialFile(const QStringList& args);

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
  AppSettings& _appSettings;
  BudgetData& _budgetData;
  CategoryController& _categoryController;
  NavigationController& _navController;
  RuleController& _ruleController;
  QUndoStack& _undoStack;
};
