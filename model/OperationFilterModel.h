#pragma once

#include <QSortFilterProxyModel>

#include "Account.h"

class OperationFilterModel : public QSortFilterProxyModel {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(Account* account READ account WRITE setAccount NOTIFY accountChanged)
  Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
  Q_PROPERTY(int currentOperationIndex READ currentOperationIndex NOTIFY currentOperationChanged)

public:
  explicit OperationFilterModel(QObject* parent = nullptr);

  Account* account() const { return _account; }
  void setAccount(Account* account);
  QString query() const { return _query; }
  void setQuery(const QString& query);
  int currentOperationIndex() const;

  Q_INVOKABLE QObject* operationAt(int index) const;
  Q_INVOKABLE void selectAt(int index, bool extend = false);
  Q_INVOKABLE void toggleSelectionAt(int index);
  Q_INVOKABLE void selectRange(int fromIndex, int toIndex);
  Q_INVOKABLE void previousOperation(bool extendSelection = false);
  Q_INVOKABLE void nextOperation(bool extendSelection = false);
  Q_INVOKABLE void moveOperation(int offset, bool extendSelection = false);

signals:
  void accountChanged();
  void queryChanged();
  void currentOperationChanged();
  void countChanged();

protected:
  bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
  bool amountMatches(double amount, const QString& query) const;
  QString searchableAmount(double amount) const;
  Operation* operationAtInternal(int index) const;
  void updateCurrentOperation();

  Account* _account = nullptr;
  Operation* _selectionAnchor = nullptr;
  QString _query;
};
