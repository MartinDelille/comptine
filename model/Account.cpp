#include <algorithm>

#include "Account.h"
#include "Operation.h"

Account::Account(const QString& name, QObject* parent) :
    QAbstractListModel(parent),
    _name(name) {
  connect(this, &Account::selectionChanged,
          this, [this]() {
            emit dataChanged(createIndex(0, 0),
                             createIndex(rowCount() - 1, 0),
                             { SelectedRole });
          });
}

Account::~Account() {
  clear();
}

Operation* Account::currentOperation() const {
  return _currentOperation;
}

int Account::rowCount(const QModelIndex& parent) const {
  if (parent.isValid())
    return 0;
  return _operations.size();
}

QVariant Account::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  int row = index.row();
  auto op = operationAt(row);

  if (!op) {
    return QVariant();
  }

  switch (static_cast<Roles>(role)) {
    case DateRole:
      return op->date();
    case AmountRole:
      return op->amount();
    case LabelRole:
      return op->label();
    case BalanceRole:
      return (row >= 0 && row < _balances.size()) ? _balances[row] : 0.0;
    case SelectedRole:
      return isSelectedAt(row);
    case OperationRole:
      return QVariant::fromValue(op);
  }
  return QVariant();
}

bool Account::setData(const QModelIndex& index, const QVariant& value,
                      int role) {
  if (!index.isValid() || role != SelectedRole)
    return false;

  const int row = index.row();
  bool selected = value.toBool();

  if (selected) {
    selectAt(row, false);
  } else {
    toggleSelectionAt(row);  // Toggle off if already selected
  }

  return true;
}

QHash<int, QByteArray> Account::roleNames() const {
  return {
    { DateRole, "date" },
    { AmountRole, "amount" },
    { LabelRole, "label" },
    { BalanceRole, "balance" },
    { SelectedRole, "selected" },
    { OperationRole, "operation" },
  };
}

void Account::refresh() {
  beginResetModel();
  recalculateBalances();
  endResetModel();
}

QStringList Account::importSourcePrefixes() const {
  return _importSources;
}

void Account::addImportSourcePrefix(const QString& filename) {
  if (!filename.isEmpty() && !_importSources.contains(filename)) {
    _importSources.append(filename);
    emit importSourcePrefixesChanged();
  }
}

void Account::setImportSourcePrefixes(const QStringList& sources) {
  _importSources = sources;
  emit importSourcePrefixesChanged();
}

int Account::currentOperationIndex() const {
  if (!_currentOperation) return -1;
  return _operations.indexOf(_currentOperation);
}

void Account::set_currentOperationIndex(int index) {
  select(operationAt(index));
}

int Account::operationIndex(Operation* operation) const {
  if (!operation) return -1;
  return _operations.indexOf(operation);
}

QList<Operation*> Account::operations() const {
  return _operations;
}

Operation* Account::addOperation(Operation* operation, bool sort) {
  if (operation == nullptr) {
    return nullptr;
  }
  int insertIndex = _operations.size();
  if (sort) {
    insertIndex = 0;
    // Insert in sorted order (most recent first)
    // For same-date operations, insert at the END of the same-date group to preserve order
    while (insertIndex < _operations.size() && _operations[insertIndex]->date() > operation->date()) {
      insertIndex++;
    }
    // Skip past any operations with the same date (insert after them)
    while (insertIndex < _operations.size() && _operations[insertIndex]->date() == operation->date()) {
      insertIndex++;
    }
  }
  beginInsertRows(QModelIndex(), insertIndex, insertIndex);
  operation->setParent(this);
  _operations.insert(insertIndex, operation);
  endInsertRows();
  recalculateBalances();
  connect(operation, &Operation::amountChanged, this, &Account::recalculateBalances);
  connect(operation, &Operation::dateChanged, this, &Account::operationDataChanged);
  connect(operation, &Operation::budgetDateChanged, this, &Account::operationDataChanged);
  emit countChanged();
  emit operationDataChanged();
  return operation;
}

bool Account::removeOperation(Operation* operation) {
  if (operation == nullptr) {
    return false;
  }
  int index = _operations.indexOf(operation);
  if (index < 0) {
    return false;
  }
  bool wasSelected = _selectedOperations.remove(operation);
  beginRemoveRows(QModelIndex(), index, index);
  _operations.removeOne(operation);
  operation->setParent(nullptr);
  // Clear from selection if present
  // Clear currentOperation if it was the removed one
  if (_currentOperation == operation) {
    _currentOperation = nullptr;
    emit currentOperationChanged();
  }
  endRemoveRows();
  recalculateBalances();
  emit countChanged();
  if (wasSelected) {
    emit selectionChanged();
  }
  emit operationDataChanged();
  return true;
}

void Account::clear() {
  bool hadSelection = !_selectedOperations.isEmpty();
  beginResetModel();
  _selectedOperations.clear();
  qDeleteAll(_operations);
  _operations.clear();
  if (_currentOperation) {
    _currentOperation = nullptr;
    emit currentOperationChanged();
  }
  endResetModel();
  emit countChanged();
  if (hadSelection) {
    emit selectionChanged();
  }
  emit operationDataChanged();
}

void Account::sortOperations() {
  beginResetModel();
  std::stable_sort(_operations.begin(), _operations.end(), [](Operation* a, Operation* b) {
    return a->date() > b->date();  // Most recent first, preserve relative order for same date
  });
  endResetModel();
  recalculateBalances();
  // The index of currentOperation may have changed after sorting
  // Selection is pointer-based so no update needed, but we need to notify
  // so that the model can update SelectedRole for affected indices
  if (_currentOperation) {
    emit currentOperationChanged();
  }
  if (!_selectedOperations.isEmpty()) {
    emit selectionChanged();
  }
}

bool Account::hasOperation(const QDate& date, double amount, const QString& label) const {
  for (auto op : _operations) {
    if (op->date() == date && op->amount() == amount && op->label() == label) {
      return true;
    }
  }
  return false;
}

Operation* Account::operationAt(int index) const {
  if (index >= 0 && index < _operations.size()) {
    return _operations[index];
  }
  return nullptr;
}

// Selection management

bool Account::isSelected(Operation* operation) const {
  return operation && _selectedOperations.contains(operation);
}

bool Account::isSelectedAt(int index) const {
  return isSelected(operationAt(index));
}

void Account::select(Operation* operation, bool extend) {
  if (!operation || !_operations.contains(operation))
    return;

  if (!extend) {
    // Clear existing selection and select only this operation
    _selectedOperations.clear();
    _selectedOperations.insert(operation);
  } else {
    // Extend selection from currentOperation to this operation
    if (_currentOperation && _operations.contains(_currentOperation)) {
      int fromIndex = _operations.indexOf(_currentOperation);
      int toIndex = _operations.indexOf(operation);
      int start = qMin(fromIndex, toIndex);
      int end = qMax(fromIndex, toIndex);
      for (int i = start; i <= end; ++i) {
        _selectedOperations.insert(_operations[i]);
      }
    } else {
      _selectedOperations.insert(operation);
    }
  }

  _currentOperation = operation;
  emit currentOperationChanged();
  emit selectionChanged();
}

void Account::previousOperation(bool extendSelection) {
  select(operationAt(currentOperationIndex() - 1), extendSelection);
}

void Account::nextOperation(bool extendSelection) {
  select(operationAt(currentOperationIndex() + 1), extendSelection);
}

void Account::selectAt(int index, bool extend) {
  select(operationAt(index), extend);
}

void Account::toggleSelection(Operation* operation) {
  if (!operation || !_operations.contains(operation))
    return;

  if (_selectedOperations.contains(operation)) {
    _selectedOperations.remove(operation);
  } else {
    _selectedOperations.insert(operation);
  }

  emit selectionChanged();
}

void Account::toggleSelectionAt(int index) {
  toggleSelection(operationAt(index));
}

void Account::selectRange(int fromIndex, int toIndex) {
  int start = qMax(0, qMin(fromIndex, toIndex));
  int end = qMin(_operations.size() - 1, qMax(fromIndex, toIndex));

  for (int i = start; i <= end; ++i) {
    _selectedOperations.insert(_operations[i]);
  }

  emit selectionChanged();
}

void Account::selectAll() {
  selectRange(0, _operations.size() - 1);
}

void Account::clearSelection() {
  if (_selectedOperations.isEmpty())
    return;

  _selectedOperations.clear();
  emit selectionChanged();
}

int Account::selectionCount() const {
  return _selectedOperations.size();
}

double Account::selectedTotal() const {
  double total = 0.0;
  for (auto op : _selectedOperations) {
    total += op->amount();
  }
  return total;
}

QSet<Operation*> Account::selectedOperations() const {
  return _selectedOperations;
}

QString Account::selectedOperationsAsCsv() const {
  if (_selectedOperations.isEmpty())
    return QString();

  // Collect selected operations in sorted order (by their index in _operations)
  QList<Operation*> sortedSelected;
  for (auto op : _operations) {
    if (_selectedOperations.contains(op)) {
      sortedSelected.append(op);
    }
  }

  QString csv;
  csv += "Date,Label,Amount,Category\n";

  for (auto op : sortedSelected) {
    csv += QString("%0,\"%1\",%2,%3\n")
               .arg(op->date().toString("yyyy-MM-dd"),
                    op->label().replace("\"", "\"\""))
               .arg(op->amount(), 0, 'f', 2)
               .arg((op->categoryDisplay()));
  }

  return csv;
}

int Account::countOperationsWithCategory(const Category* category) const {
  auto hasCategory = [category](const Operation* operation) {
    return operation->amountForCategory(category);
  };
  return std::count_if(_operations.begin(), _operations.end(), hasCategory);
}

double Account::currentBalance() const {
  if (_balances.isEmpty())
    return 0.0;
  return _balances.first();
}

double Account::balanceAt(int index) const {
  if (index < 0 || index >= _balances.size())
    return 0.0;
  return _balances[index];
}

void Account::recalculateBalances() {
  _balances.clear();

  const int count = rowCount();

  if (count == 0)
    return;

  _balances.resize(count);

  // Operations are sorted most recent first
  // Calculate cumulative balance from oldest to newest
  double balance = 0.0;
  for (int i = count - 1; i >= 0; --i) {
    auto op = operationAt(i);
    if (op) {
      balance += op->amount();
    }
    _balances[i] = balance;
  }
  emit dataChanged(createIndex(0, 0), createIndex(count - 1, 0), { BalanceRole });
  emit balanceChanged();
}
