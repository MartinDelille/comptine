#include "AccountListModel.h"

AccountListModel::AccountListModel(QList<Account*>& accounts, QObject* parent) :
    QAbstractListModel(parent),
    _accounts(accounts) {
}

int AccountListModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid())
    return 0;

  return _accounts.size();
}

QVariant AccountListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid())
    return QVariant();

  const int row = index.row();
  const int accountCount = _accounts.size();

  if (row < 0 || row >= accountCount)
    return QVariant();

  Account* account = _accounts.at(row);
  if (!account)
    return QVariant();

  switch (static_cast<Roles>(role)) {
    case NameRole:
      return account->name();
    case OperationCountRole:
      return account->operationCount();
    case AccountRole:
      return QVariant::fromValue(account);
  }
  return QVariant();
}

QHash<int, QByteArray> AccountListModel::roleNames() const {
  return {
    { NameRole, "name" },
    { OperationCountRole, "operationCount" },
    { AccountRole, "account" }
  };
}

void AccountListModel::refresh() {
  beginResetModel();
  endResetModel();
  emit countChanged();
}
