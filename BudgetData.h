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

class NavigationController;

class BudgetData : public QObject {
  Q_OBJECT

  Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
  Q_PROPERTY(QString appCommitHash READ appCommitHash CONSTANT)

  // Read-only computed properties (macro-generated, implemented in .cpp)
  PROPERTY_RO(int, accountCount)

  // Models exposed to QML
  Q_PROPERTY(OperationListModel* operationModel READ operationModel CONSTANT)
  Q_PROPERTY(AccountListModel* accountModel READ accountModel CONSTANT)
  Q_PROPERTY(QUndoStack* undoStack READ undoStack CONSTANT)
  Q_PROPERTY(Account* currentAccount READ currentAccount NOTIFY currentAccountChanged)

public:
  explicit BudgetData(QObject* parent = nullptr);
  ~BudgetData();

  // Version info accessors
  QString appVersion() const;
  QString appCommitHash() const;

  // Model accessors
  OperationListModel* operationModel() const { return _operationModel; }
  AccountListModel* accountModel() const { return _accountModel; }
  QUndoStack* undoStack() const { return _undoStack; }
  Account* currentAccount() const;

  // Account management
  QList<Account*> accounts() const;
  Q_INVOKABLE Account* getAccount(int index) const;
  Q_INVOKABLE Account* getAccountByName(const QString& name) const;
  Q_INVOKABLE void renameCurrentAccount(const QString& newName);
  void addAccount(Account* account);
  void removeAccount(int index);
  void clearAccounts();

  // Operation editing
  Q_INVOKABLE void setOperationCategory(int operationIndex, const QString& newCategory);
  Q_INVOKABLE void setOperationBudgetDate(int operationIndex, const QDate& newBudgetDate);
  Q_INVOKABLE void setOperationAmount(int operationIndex, double newAmount);
  Q_INVOKABLE void setOperationDate(int operationIndex, const QDate& newDate);
  Q_INVOKABLE void setOperationDescription(int operationIndex, const QString& newDescription);
  Q_INVOKABLE void splitOperation(int operationIndex, const QVariantList& allocations);

  // Clear all data (called by FileController)
  void clear();

  // Undo/Redo
  Q_INVOKABLE void undo();
  Q_INVOKABLE void redo();

  // Set reference to NavigationController (for current account access)
  void setNavigationController(NavigationController* navController);

signals:
  void operationDataChanged();   // Emitted when operation data changes (e.g., category edit)
  void currentAccountChanged();  // Emitted when current account changes (for QML binding)

private:
  QList<Account*> _accounts;
  OperationListModel* _operationModel;
  AccountListModel* _accountModel;
  QUndoStack* _undoStack;
  NavigationController* _navController = nullptr;
};
