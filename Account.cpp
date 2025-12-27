#include "Account.h"

Account::Account(QObject* parent) :
    QObject(parent) {}

Account::Account(const QString& name, QObject* parent) :
    QObject(parent), _name(name) {}

int Account::currentOperationIndex() const {
  if (!_currentOperation) return -1;
  return _operations.indexOf(_currentOperation);
}

void Account::set_currentOperationIndex(int index) {
  set_currentOperation(getOperation(index));
}

int Account::operationCount() const {
  return _operations.size();
}

QList<Operation*> Account::operations() const {
  return _operations;
}

void Account::addOperation(Operation* operation) {
  if (operation) {
    operation->setParent(this);
    // Insert in sorted order (most recent first)
    // For same-date operations, insert at the END of the same-date group to preserve order
    int insertIndex = 0;
    while (insertIndex < _operations.size() && _operations[insertIndex]->date() > operation->date()) {
      insertIndex++;
    }
    // Skip past any operations with the same date (insert after them)
    while (insertIndex < _operations.size() && _operations[insertIndex]->date() == operation->date()) {
      insertIndex++;
    }
    _operations.insert(insertIndex, operation);
    emit operationCountChanged();
  }
}

void Account::appendOperation(Operation* operation) {
  if (operation) {
    operation->setParent(this);
    // Append without sorting - preserves file order when loading
    _operations.append(operation);
    emit operationCountChanged();
  }
}

void Account::removeOperation(int index) {
  if (index >= 0 && index < _operations.size()) {
    Operation* op = _operations.takeAt(index);
    // Clear from selection if present
    bool wasSelected = _selectedOperations.remove(op);
    // Clear currentOperation if it was the deleted one
    if (_currentOperation == op) {
      set_currentOperation(nullptr);
    }
    delete op;
    emit operationCountChanged();
    if (wasSelected) {
      emit selectionChanged();
    }
  }
}

bool Account::removeOperation(Operation* operation) {
  int index = _operations.indexOf(operation);
  if (index >= 0) {
    _operations.removeAt(index);
    // Clear from selection if present
    bool wasSelected = _selectedOperations.remove(operation);
    // Clear currentOperation if it was the removed one
    if (_currentOperation == operation) {
      set_currentOperation(nullptr);
    }
    emit operationCountChanged();
    if (wasSelected) {
      emit selectionChanged();
    }
    return true;
  }
  return false;
}

void Account::clearOperations() {
  bool hadSelection = !_selectedOperations.isEmpty();
  _selectedOperations.clear();
  qDeleteAll(_operations);
  _operations.clear();
  if (_currentOperation) {
    set_currentOperation(nullptr);
  }
  emit operationCountChanged();
  if (hadSelection) {
    emit selectionChanged();
  }
}

void Account::sortOperations() {
  std::stable_sort(_operations.begin(), _operations.end(), [](Operation* a, Operation* b) {
    return a->date() > b->date();  // Most recent first, preserve relative order for same date
  });
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
  for (Operation* op : _operations) {
    if (op->date() == date && op->amount() == amount && op->label() == label) {
      return true;
    }
  }
  return false;
}

Operation* Account::getOperation(int index) const {
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
  return isSelected(getOperation(index));
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

  emit selectionChanged();
}

void Account::selectAt(int index, bool extend) {
  select(getOperation(index), extend);
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
  toggleSelection(getOperation(index));
}

void Account::selectRange(int fromIndex, int toIndex) {
  int start = qMax(0, qMin(fromIndex, toIndex));
  int end = qMin(_operations.size() - 1, qMax(fromIndex, toIndex));

  for (int i = start; i <= end; ++i) {
    _selectedOperations.insert(_operations[i]);
  }

  emit selectionChanged();
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
  for (Operation* op : _selectedOperations) {
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
  for (Operation* op : _operations) {
    if (_selectedOperations.contains(op)) {
      sortedSelected.append(op);
    }
  }

  QString csv;
  csv += "Date,Label,Amount,Category\n";

  for (Operation* op : sortedSelected) {
    csv += QString("%0,\"%1\",%2,%3\n")
               .arg(op->date().toString("yyyy-MM-dd"))
               .arg(op->label().replace("\"", "\"\""))
               .arg(op->amount(), 0, 'f', 2)
               .arg((op->categoryDisplay()));
  }

  return csv;
}
