#include "OperationListModel.h"

OperationListModel::OperationListModel(QObject *parent) :
    QAbstractListModel(parent) {}

int OperationListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid() || !_account)
    return 0;
  return _account->operationCount();
}

QVariant OperationListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || !_account)
    return QVariant();

  const int row = index.row();
  if (row < 0 || row >= _account->operationCount())
    return QVariant();

  Operation *op = _account->getOperation(row);
  if (!op)
    return QVariant();

  switch (role) {
    case DateRole:
      return op->date();
    case AmountRole:
      return op->amount();
    case DescriptionRole:
      return op->description();
    case CategoryRole:
      return op->category();
    case BalanceRole:
      return (row >= 0 && row < _balances.size()) ? _balances[row] : 0.0;
    case SelectedRole:
      return _selection.contains(row);
    case OperationRole:
      return QVariant::fromValue(op);
    default:
      return QVariant();
  }
}

bool OperationListModel::setData(const QModelIndex &index, const QVariant &value,
                                 int role) {
  if (!index.isValid() || role != SelectedRole)
    return false;

  const int row = index.row();
  bool selected = value.toBool();

  if (selected) {
    _selection.insert(row);
  } else {
    _selection.remove(row);
  }

  emit dataChanged(index, index, { SelectedRole });
  emit selectionChanged();
  return true;
}

QHash<int, QByteArray> OperationListModel::roleNames() const {
  return {
    { DateRole, "date" },
    { AmountRole, "amount" },
    { DescriptionRole, "description" },
    { CategoryRole, "category" },
    { BalanceRole, "balance" },
    { SelectedRole, "selected" },
    { OperationRole, "operation" }
  };
}

void OperationListModel::setAccount(Account *account) {
  if (_account == account)
    return;

  beginResetModel();

  // Disconnect from old account
  if (_account) {
    disconnect(_account, nullptr, this, nullptr);
  }

  _account = account;
  _selection.clear();
  _lastClickedIndex = -1;

  // Connect to new account
  if (_account) {
    connect(_account, &Account::operationCountChanged, this,
            &OperationListModel::onOperationCountChanged);
  }

  recalculateBalances();
  endResetModel();

  emit countChanged();
  emit selectionChanged();
}

void OperationListModel::onOperationCountChanged() {
  beginResetModel();
  _selection.clear();
  _lastClickedIndex = -1;
  recalculateBalances();
  endResetModel();
  emit countChanged();
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
    Operation *op = _account->getOperation(i);
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

Operation *OperationListModel::operationAt(int index) const {
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
  if (index < 0 || !_account || index >= _account->operationCount())
    return;

  if (!extend) {
    // Clear existing selection
    QSet<int> oldSelection = _selection;
    _selection.clear();
    _selection.insert(index);

    // Emit dataChanged for deselected items
    for (int i : oldSelection) {
      if (i != index) {
        QModelIndex mi = createIndex(i, 0);
        emit dataChanged(mi, mi, { SelectedRole });
      }
    }
  } else {
    // Extend selection from last clicked to current
    if (_lastClickedIndex >= 0) {
      int from = qMin(_lastClickedIndex, index);
      int to = qMax(_lastClickedIndex, index);
      for (int i = from; i <= to; ++i) {
        _selection.insert(i);
      }
      // Emit dataChanged for range
      emit dataChanged(createIndex(from, 0), createIndex(to, 0), { SelectedRole });
    } else {
      _selection.insert(index);
    }
  }

  _lastClickedIndex = index;

  // Emit dataChanged for newly selected item
  QModelIndex mi = createIndex(index, 0);
  emit dataChanged(mi, mi, { SelectedRole });
  emit selectionChanged();
}

void OperationListModel::toggleSelection(int index) {
  if (index < 0 || !_account || index >= _account->operationCount())
    return;

  if (_selection.contains(index)) {
    _selection.remove(index);
  } else {
    _selection.insert(index);
  }

  _lastClickedIndex = index;

  QModelIndex mi = createIndex(index, 0);
  emit dataChanged(mi, mi, { SelectedRole });
  emit selectionChanged();
}

void OperationListModel::selectRange(int fromIndex, int toIndex) {
  if (!_account)
    return;

  int from = qMax(0, qMin(fromIndex, toIndex));
  int to = qMin(_account->operationCount() - 1, qMax(fromIndex, toIndex));

  for (int i = from; i <= to; ++i) {
    _selection.insert(i);
  }

  emit dataChanged(createIndex(from, 0), createIndex(to, 0), { SelectedRole });
  emit selectionChanged();
}

void OperationListModel::clearSelection() {
  if (_selection.isEmpty())
    return;

  QSet<int> oldSelection = _selection;
  _selection.clear();
  _lastClickedIndex = -1;

  for (int i : oldSelection) {
    QModelIndex mi = createIndex(i, 0);
    emit dataChanged(mi, mi, { SelectedRole });
  }

  emit selectionChanged();
}

bool OperationListModel::isSelected(int index) const {
  return _selection.contains(index);
}

double OperationListModel::selectedTotal() const {
  if (!_account || _selection.isEmpty())
    return 0.0;

  double total = 0.0;
  for (int index : _selection) {
    Operation *op = _account->getOperation(index);
    if (op) {
      total += op->amount();
    }
  }
  return total;
}

QString OperationListModel::selectedOperationsAsCsv() const {
  if (!_account || _selection.isEmpty())
    return QString();

  QString csv;
  csv += "Date,Description,Amount,Category\n";

  // Sort indices for consistent output
  QList<int> sortedIndices = _selection.values();
  std::sort(sortedIndices.begin(), sortedIndices.end());

  for (int index : sortedIndices) {
    Operation *op = _account->getOperation(index);
    if (op) {
      csv += QString("%1,\"%2\",%3,%4\n")
                 .arg(op->date().toString("yyyy-MM-dd"))
                 .arg(op->description().replace("\"", "\"\""))
                 .arg(op->amount(), 0, 'f', 2)
                 .arg(op->category());
    }
  }

  return csv;
}

void OperationListModel::emitSelectionDataChanged() {
  for (int i : _selection) {
    QModelIndex mi = createIndex(i, 0);
    emit dataChanged(mi, mi, { SelectedRole });
  }
}
