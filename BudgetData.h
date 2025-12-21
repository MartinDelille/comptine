#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QUndoStack>
#include <QVariant>

#include "Account.h"
#include "AccountListModel.h"
#include "OperationListModel.h"
#include "PropertyMacros.h"

class CategoryController;
class NavigationController;

class BudgetData : public QObject {
  Q_OBJECT

  // Read-only computed properties (macro-generated, implemented in .cpp)
  PROPERTY_RO(int, accountCount)

  // Models exposed to QML
  Q_PROPERTY(OperationListModel* operationModel READ operationModel CONSTANT)
  Q_PROPERTY(AccountListModel* accountModel READ accountModel CONSTANT)

  Q_PROPERTY(Account* currentAccount READ currentAccount NOTIFY currentAccountChanged)

public:
  explicit BudgetData(QUndoStack& undoStack);
  ~BudgetData();

  // Model accessors
  OperationListModel* operationModel() const { return _operationModel; }
  AccountListModel* accountModel() const { return _accountModel; }

  Account* currentAccount() const;

  // Account management
  QList<Account*> accounts() const;
  Q_INVOKABLE Account* getAccount(int index) const;
  Q_INVOKABLE Account* getAccountByName(const QString& name) const;
  Q_INVOKABLE void renameCurrentAccount(const QString& newName);
  void addAccount(Account* account);
  void removeAccount(int index);
  Account* takeAccount(Account* account);  // Remove without deleting, returns nullptr if not found
  void selectAccount(int index);           // Select account by index (updates navigation)
  void clearAccounts();

  // Operation editing
  Q_INVOKABLE void setOperationCategory(Operation* operation, const Category* newCategory);
  Q_INVOKABLE void setOperationBudgetDate(Operation* operation, const QDate& newBudgetDate);
  Q_INVOKABLE void setOperationAmount(Operation* operation, double newAmount);
  Q_INVOKABLE void setOperationDate(Operation* operation, const QDate& newDate);
  Q_INVOKABLE void setOperationDescription(Operation* operation, const QString& newDescription);
  Q_INVOKABLE void splitOperation(Operation* operation, const QVariantList& allocations);

  // Clear all data (called by FileController)
  void clear();

  // Undo/Redo
  Q_INVOKABLE void undo();
  Q_INVOKABLE void redo();

  // Set reference to NavigationController (for current account access)
  void setNavigationController(NavigationController* navController);

  // Set reference to CategoryController (for clearing categories)
  void setCategoryController(CategoryController* categoryController);

signals:
  void operationDataChanged();   // Emitted when operation data changes (e.g., category edit)
  void currentAccountChanged();  // Emitted when current account changes (for QML binding)

private:
  QUndoStack& _undoStack;
  QList<Account*> _accounts;
  OperationListModel* _operationModel;
  AccountListModel* _accountModel;
  NavigationController* _navController = nullptr;
  CategoryController* _categoryController = nullptr;
};
