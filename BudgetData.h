#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QUndoStack>
#include "Account.h"
#include "PropertyMacros.h"

class BudgetData : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT

  PROPERTY_RW(Account*, currentAccount, nullptr)
  Q_PROPERTY(int accountCount READ rowCount NOTIFY accountCountChanged)

  Q_PROPERTY(int currentAccountIndex READ currentAccountIndex WRITE set_currentAccountIndex NOTIFY currentAccountChanged)

public:
  enum Roles {
    NameRole = Qt::UserRole + 1,
    OperationCountRole,
    AccountRole
  };
  Q_ENUM(Roles)

  explicit BudgetData(QUndoStack& undoStack);
  ~BudgetData();

  // Model accessors
  int currentAccountIndex() const;
  void set_currentAccountIndex(int index);

  // QAbstractListModel interface
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  // Account management
  QList<Account*> accounts() const;
  Q_INVOKABLE Account* accountAt(int index) const;
  Q_INVOKABLE Account* accountByName(const QString& name) const;
  Q_INVOKABLE QString suggestedAccountForFile(const QString& filename) const;
  Q_INVOKABLE int accountIndex(Account* account) const;
  Q_INVOKABLE void renameCurrentAccount(const QString& newName);
  void addAccount(Account* account);
  void removeAccount(int index);
  Account* takeAccount(Account* account);  // Remove without deleting, returns nullptr if not found
  void clearAccounts();

  // Operation editing
  Q_INVOKABLE void addOperation(const QDate& date, double amount, const QString& label, const QString& details, const QList<Allocation*>& allocations);
  Q_INVOKABLE void setOperationBudgetDate(Operation* operation, const QDate& newBudgetDate);
  Q_INVOKABLE void setOperationAmount(Operation* operation, double newAmount);
  Q_INVOKABLE void setOperationDate(Operation* operation, const QDate& newDate);
  Q_INVOKABLE void setOperationLabel(Operation* operation, const QString& newLabel);
  Q_INVOKABLE void setOperationDetails(Operation* operation, const QString& newDetails);
  Q_INVOKABLE void setOperationAllocations(Operation* operation, const QList<Allocation*>& allocations);
  Q_INVOKABLE Operation* createCounterPart(Operation* operation, Account* targetAccount, const QString& categoryName);
  Q_INVOKABLE void deleteSelectedOperations();

  // Clear all data (called by FileController)
  void clear();

  // Copy selected operations to system clipboard as CSV
  Q_INVOKABLE void copySelectedOperations() const;

signals:
  void accountCountChanged();
  void operationDataChanged();  // Emitted when operation data changes (e.g., category edit)

private:
  QUndoStack& _undoStack;
  QList<Account*> _accounts;
};
