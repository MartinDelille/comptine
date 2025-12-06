#include "AccountListModel.h"

AccountListModel::AccountListModel(QObject *parent)
    : QAbstractListModel(parent) {}

int AccountListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid() || !_accounts)
    return 0;

  return _accounts->size();
}

QVariant AccountListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || !_accounts)
    return QVariant();

  const int row = index.row();
  const int accountCount = _accounts->size();

  if (row < 0 || row >= accountCount)
    return QVariant();

  Account *account = _accounts->at(row);
  if (!account)
    return QVariant();

  switch (role) {
  case NameRole:
    return account->name();
  case OperationCountRole:
    return account->operationCount();
  case AccountRole:
    return QVariant::fromValue(account);
  default:
    return QVariant();
  }
}

QHash<int, QByteArray> AccountListModel::roleNames() const {
  return {
      {NameRole, "name"},
      {OperationCountRole, "operationCount"},
      {AccountRole, "account"}
  };
}

void AccountListModel::setAccounts(QList<Account *> *accounts) {
  if (_accounts == accounts)
    return;

  beginResetModel();
  _accounts = accounts;
  endResetModel();

  emit countChanged();
}

void AccountListModel::refresh() {
  beginResetModel();
  endResetModel();
  emit countChanged();
}
