#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QUndoStack>
#include <QVariant>
#include "Account.h"
#include "AccountListModel.h"
#include "Category.h"
#include "OperationListModel.h"
#include "PropertyMacros.h"

class BudgetData : public QObject {
  Q_OBJECT

  // UI state properties (macro-generated)
  PROPERTY_RW(int, currentTabIndex, 0)
  PROPERTY_RW(int, budgetYear, 0)
  PROPERTY_RW(int, budgetMonth, 0)

  // Data properties (macro-generated)
  PROPERTY_RW(QString, currentFilePath, {})

  // Read-only computed properties (macro-generated, implemented in .cpp)
  PROPERTY_RO(int, accountCount)
  PROPERTY_RO(int, categoryCount)
  PROPERTY_RO(Account *, currentAccount)

  // Custom property with validation logic (implemented in .cpp)
  PROPERTY_RW_CUSTOM(int, currentAccountIndex, -1)

  // Models exposed to QML
  Q_PROPERTY(OperationListModel *operationModel READ operationModel CONSTANT)
  Q_PROPERTY(AccountListModel *accountModel READ accountModel CONSTANT)
  Q_PROPERTY(QUndoStack *undoStack READ undoStack CONSTANT)

public:
  explicit BudgetData(QObject *parent = nullptr);
  ~BudgetData();

  // Model accessors
  OperationListModel *operationModel() const { return _operationModel; }
  AccountListModel *accountModel() const { return _accountModel; }
  QUndoStack *undoStack() const { return _undoStack; }

  // Account management
  QList<Account *> accounts() const;
  Q_INVOKABLE Account *getAccount(int index) const;
  Q_INVOKABLE Account *getAccountByName(const QString &name) const;
  Q_INVOKABLE void renameCurrentAccount(const QString &newName);
  void addAccount(Account *account);
  void removeAccount(int index);
  void clearAccounts();

  // Category management
  QList<Category *> categories() const;
  Q_INVOKABLE Category *getCategory(int index) const;
  Q_INVOKABLE Category *getCategoryByName(const QString &name) const;
  Q_INVOKABLE void editCategory(int index, const QString &newName, double newBudgetLimit);
  Q_INVOKABLE QStringList categoryNames() const;
  void addCategory(Category *category);
  void removeCategory(int index);
  Category *takeCategoryByName(const QString &name);  // Remove without deleting
  void clearCategories();

  // Operation category editing
  Q_INVOKABLE void setOperationCategory(int operationIndex, const QString &newCategory);
  Q_INVOKABLE void setOperationBudgetDate(int operationIndex, const QDate &newBudgetDate);

  // Budget calculations (aggregates across all accounts)
  Q_INVOKABLE double spentInCategory(const QString &categoryName, int year, int month) const;
  Q_INVOKABLE QVariantList monthlyBudgetSummary(int year, int month) const;

  // File operations
  Q_INVOKABLE bool loadFromYaml(const QString &filePath);
  Q_INVOKABLE bool saveToYaml(const QString &filePath) const;
  Q_INVOKABLE bool importFromCsv(const QString &filePath,
                                 const QString &accountName = QString(),
                                 bool useCategories = false);

  // Clear all data
  Q_INVOKABLE void clear();

  // Clipboard operations (delegates to operationModel)
  Q_INVOKABLE void copySelectedOperationsToClipboard() const;

  // Undo/Redo
  Q_INVOKABLE void undo();
  Q_INVOKABLE void redo();

signals:
  void dataLoaded();      // Emitted after any data load (YAML or CSV import)
  void yamlFileLoaded();  // Emitted only after YAML file load (for UI state restore)
  void dataSaved();
  void operationDataChanged();  // Emitted when operation data changes (e.g., category edit)

private:
  QList<Account *> _accounts;
  QList<Category *> _categories;
  OperationListModel *_operationModel;
  AccountListModel *_accountModel;
  QUndoStack *_undoStack;
};
