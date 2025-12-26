#pragma once

#include <QtQml/qqml.h>
#include <QAbstractListModel>
#include <QPointer>

#include "Account.h"

class OperationListModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
  Q_PROPERTY(int selectionCount READ selectionCount NOTIFY selectionChanged)
  Q_PROPERTY(double selectedTotal READ selectedTotal NOTIFY selectionChanged)

public:
  enum Roles {
    DateRole = Qt::UserRole + 1,
    AmountRole,
    LabelRole,
    BalanceRole,
    SelectedRole,
    OperationRole
  };
  Q_ENUM(Roles)

  explicit OperationListModel(QObject* parent = nullptr);

  // QAbstractListModel interface
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role) override;
  QHash<int, QByteArray> roleNames() const override;

  // Account management
  void setAccount(Account* account);
  Account* account() const { return _account; }

  // Selection management (delegates to Account)
  Q_INVOKABLE void select(int index, bool extend = false);
  Q_INVOKABLE void toggleSelection(int index);
  Q_INVOKABLE void selectRange(int fromIndex, int toIndex);
  Q_INVOKABLE void clearSelection();
  Q_INVOKABLE bool isSelected(int index) const;
  void selectByPointer(Operation* operation);  // Select by operation pointer (finds current index)
  int selectionCount() const;
  double selectedTotal() const;
  QString selectedOperationsAsCsv() const;

  // Refresh balances after data changes
  void refresh();

  // QML helper methods to avoid magic role numbers
  Q_INVOKABLE Operation* operationAt(int index) const;
  Q_INVOKABLE double balanceAt(int index) const;

signals:
  void countChanged();
  void selectionChanged();
  void operationDataChanged();       // Emitted when operation data is modified (used by undo commands)
  void operationFocused(int index);  // Emitted when selectByPointer() programmatically selects an operation

private:
  QPointer<Account> _account;
  QVector<double> _balances;

  void recalculateBalances();
  void onOperationCountChanged();
  void onAccountSelectionChanged();
};
