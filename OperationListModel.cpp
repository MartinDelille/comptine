#include "OperationListModel.h"

OperationListModel::OperationListModel(QObject* parent) :
    QAbstractListModel(parent) {}

int OperationListModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid() || !_account)
    return 0;
  return _account->operationCount();
}

QVariant OperationListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || !_account)
    return QVariant();

  const int row = index.row();
  if (row < 0 || row >= _account->operationCount())
    return QVariant();

  if (auto op = _account->getOperation(row)) {
    switch (static_cast<Roles>(role)) {
      case DateRole:
        return op->date();
      case AmountRole:
        return op->amount();
      case LabelRole:
        return op->label();
      case CategoryRole:
        return QVariant::fromValue(op->category());
      case BalanceRole:
        return (row >= 0 && row < _balances.size()) ? _balances[row] : 0.0;
      case SelectedRole:
        return _account->isSelectedAt(row);
      case OperationRole:
        return QVariant::fromValue(op);
    }
  }
  return QVariant();
}

bool OperationListModel::setData(const QModelIndex& index, const QVariant& value,
                                 int role) {
  if (!index.isValid() || !_account || role != SelectedRole)
    return false;

  const int row = index.row();
  bool selected = value.toBool();

  if (selected) {
    _account->selectAt(row, false);
  } else {
    _account->toggleSelectionAt(row);  // Toggle off if already selected
  }

  return true;
}

QHash<int, QByteArray> OperationListModel::roleNames() const {
  return {
    { DateRole, "date" },
    { AmountRole, "amount" },
    { LabelRole, "label" },
    { CategoryRole, "category" },
    { BalanceRole, "balance" },
    { SelectedRole, "selected" },
    { OperationRole, "operation" }
  };
}

void OperationListModel::setAccount(Account* account) {
  if (_account == account) {
    return;
  }

  // Disconnect from old account
  if (_account) {
    disconnect(_account, nullptr, this, nullptr);
  }

  _account = account;

  // Connect to new account
  if (_account) {
    connect(_account, &Account::operationCountChanged, this,
            &OperationListModel::onOperationCountChanged);
    connect(_account, &Account::selectionChanged, this,
            &OperationListModel::onAccountSelectionChanged);
  }

  recalculateBalances();

  beginResetModel();
  endResetModel();

  emit countChanged();
  emit selectionChanged();
}

void OperationListModel::onOperationCountChanged() {
  beginResetModel();
  recalculateBalances();
  endResetModel();
  emit countChanged();
  emit selectionChanged();
}

void OperationListModel::onAccountSelectionChanged() {
  // Notify that SelectedRole changed for all rows
  if (_account && _account->operationCount() > 0) {
    emit dataChanged(createIndex(0, 0),
                     createIndex(_account->operationCount() - 1, 0),
                     { SelectedRole });
  }
  emit selectionChanged();
}

void OperationListModel::recalculateBalances() {
  _balances.clear();

  if (!_account || _account->operationCount() == 0)
    return;

  const int count = _account->operationCount();
  _balances.resize(count);

  // Operations are sorted most recent first
  // Calculate cumulative balance from oldest to newest
  double balance = 0.0;
  for (int i = count - 1; i >= 0; --i) {
    Operation* op = _account->getOperation(i);
    if (op) {
      balance += op->amount();
    }
    _balances[i] = balance;
  }
}

void OperationListModel::refresh() {
  beginResetModel();
  recalculateBalances();
  endResetModel();
}

Operation* OperationListModel::operationAt(int index) const {
  if (!_account || index < 0 || index >= _account->operationCount())
    return nullptr;
  return _account->getOperation(index);
}

double OperationListModel::balanceAt(int index) const {
  if (index < 0 || index >= _balances.size())
    return 0.0;
  return _balances[index];
}

void OperationListModel::select(int index, bool extend) {
  if (!_account)
    return;
  _account->selectAt(index, extend);
}

void OperationListModel::toggleSelection(int index) {
  if (!_account)
    return;
  _account->toggleSelectionAt(index);
}

void OperationListModel::selectRange(int fromIndex, int toIndex) {
  if (!_account)
    return;
  _account->selectRange(fromIndex, toIndex);
}

void OperationListModel::clearSelection() {
  if (!_account)
    return;
  _account->clearSelection();
}

bool OperationListModel::isSelected(int index) const {
  if (!_account)
    return false;
  return _account->isSelectedAt(index);
}

void OperationListModel::selectByPointer(Operation* operation) {
  if (!_account || !operation)
    return;

  // Set the operation as the account's current operation and select it
  _account->set_currentOperation(operation);
  _account->select(operation, false);

  // Find the index and emit operationFocused for QML to scroll
  int index = _account->currentOperationIndex();
  if (index >= 0) {
    emit operationFocused(index);
  }
}

int OperationListModel::selectionCount() const {
  if (!_account)
    return 0;
  return _account->selectionCount();
}

double OperationListModel::selectedTotal() const {
  if (!_account)
    return 0.0;
  return _account->selectedTotal();
}

QString OperationListModel::selectedOperationsAsCsv() const {
  if (!_account)
    return QString();
  return _account->selectedOperationsAsCsv();
}
