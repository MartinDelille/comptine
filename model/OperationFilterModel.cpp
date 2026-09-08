#include <QLocale>

#include "OperationFilterModel.h"

OperationFilterModel::OperationFilterModel(QObject* parent) :
    QSortFilterProxyModel(parent) {
  setDynamicSortFilter(false);
  connect(this, &QAbstractItemModel::modelReset,
          this, &OperationFilterModel::currentOperationChanged);
  connect(this, &QAbstractItemModel::rowsInserted,
          this, &OperationFilterModel::currentOperationChanged);
  connect(this, &QAbstractItemModel::rowsRemoved,
          this, &OperationFilterModel::currentOperationChanged);
  connect(this, &QAbstractItemModel::modelReset,
          this, &OperationFilterModel::countChanged);
  connect(this, &QAbstractItemModel::rowsInserted,
          this, &OperationFilterModel::countChanged);
  connect(this, &QAbstractItemModel::rowsRemoved,
          this, &OperationFilterModel::countChanged);
}

void OperationFilterModel::setAccount(Account* account) {
  if (_account == account)
    return;

  if (_account)
    disconnect(_account, nullptr, this, nullptr);

  _account = account;
  _selectionAnchor = nullptr;
  setSourceModel(_account);

  if (_account) {
    connect(_account, &Account::operationDataChanged, this, [this]() {
      beginFilterChange();
      endFilterChange(QSortFilterProxyModel::Direction::Rows);
      updateCurrentOperation();
      emit countChanged();
    });
    connect(_account, &Account::currentOperationChanged,
            this, &OperationFilterModel::currentOperationChanged);
  }

  beginFilterChange();
  endFilterChange(QSortFilterProxyModel::Direction::Rows);
  emit accountChanged();
  emit currentOperationChanged();
}

void OperationFilterModel::setQuery(const QString& query) {
  if (_query == query)
    return;

  _query = query;
  beginFilterChange();
  endFilterChange(QSortFilterProxyModel::Direction::Rows);
  updateCurrentOperation();
  emit queryChanged();
  emit countChanged();
}

int OperationFilterModel::currentOperationIndex() const {
  if (!_account || !_account->currentOperation())
    return -1;

  const QModelIndex sourceIndex = _account->index(
      _account->operationIndex(_account->currentOperation()), 0);
  const QModelIndex proxyIndex = mapFromSource(sourceIndex);
  return proxyIndex.isValid() ? proxyIndex.row() : -1;
}

QObject* OperationFilterModel::operationAt(int index) const {
  return operationAtInternal(index);
}

void OperationFilterModel::selectAt(int index, bool extend) {
  auto* operation = operationAtInternal(index);
  if (!operation || !_account)
    return;

  if (!extend) {
    _selectionAnchor = operation;
    _account->select(operation, false);
    return;
  }

  if (!_selectionAnchor || _account->operationIndex(_selectionAnchor) < 0)
    _selectionAnchor = _account->currentOperation()
                           ? _account->currentOperation()
                           : operation;

  const QModelIndex anchorSourceIndex = _account->index(
      _account->operationIndex(_selectionAnchor), 0);
  const QModelIndex anchorProxyIndex = mapFromSource(anchorSourceIndex);
  if (anchorProxyIndex.isValid())
    selectRange(anchorProxyIndex.row(), index);
  else {
    _selectionAnchor = operation;
    _account->select(operation, false);
  }
}

void OperationFilterModel::toggleSelectionAt(int index) {
  if (auto* operation = operationAtInternal(index))
    _account->toggleSelection(operation);
}

void OperationFilterModel::selectRange(int fromIndex, int toIndex) {
  if (!_account || rowCount() == 0)
    return;

  const int start = qMax(0, qMin(fromIndex, toIndex));
  const int end = qMin(rowCount() - 1, qMax(fromIndex, toIndex));
  QList<Operation*> visibleOperations;
  visibleOperations.reserve(end - start + 1);
  const int step = toIndex >= fromIndex ? 1 : -1;
  for (int index = start; index <= end; ++index) {
    const int visibleIndex = step > 0 ? index : end - (index - start);
    visibleOperations.append(operationAtInternal(visibleIndex));
  }

  _account->selectOperations(visibleOperations, false);
}

void OperationFilterModel::previousOperation(bool extendSelection) {
  if (!_account || rowCount() == 0)
    return;

  const int current = currentOperationIndex();
  const int target = current < 0 ? rowCount() - 1 : current - 1;
  if (target >= 0)
    selectAt(target, extendSelection);
}

void OperationFilterModel::nextOperation(bool extendSelection) {
  if (!_account || rowCount() == 0)
    return;

  const int current = currentOperationIndex();
  const int target = current < 0 ? 0 : current + 1;
  if (target < rowCount())
    selectAt(target, extendSelection);
}

void OperationFilterModel::moveOperation(int offset, bool extendSelection) {
  if (!_account || rowCount() == 0 || offset == 0)
    return;

  const int current = currentOperationIndex();
  const int target = current < 0
                         ? (offset < 0 ? rowCount() - 1 : 0)
                         : qBound(0, current + offset, rowCount() - 1);
  selectAt(target, extendSelection);
}

bool OperationFilterModel::filterAcceptsRow(int sourceRow,
                                            const QModelIndex& sourceParent) const {
  if (_query.trimmed().isEmpty())
    return true;

  const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
  const auto* operation = index.data(Account::OperationRole).value<Operation*>();
  if (!operation)
    return false;

  const QString query = _query.trimmed();
  return operation->label().contains(query, Qt::CaseInsensitive) || operation->details().contains(query, Qt::CaseInsensitive) || amountMatches(operation->amount(), query);
}

bool OperationFilterModel::amountMatches(double amount, const QString& query) const {
  const bool negative = query.startsWith(QLatin1Char('-'));
  const bool positive = query.startsWith(QLatin1Char('+'));
  const QString numericQuery = (negative || positive) ? query.mid(1) : query;

  if (negative && amount >= 0)
    return false;
  if (positive && amount < 0)
    return false;

  return searchableAmount(qAbs(amount)).startsWith(numericQuery, Qt::CaseInsensitive);
}

QString OperationFilterModel::searchableAmount(double amount) const {
  const QString invariant = QString::number(amount, 'f', 2);
  const QString localized = QLocale().toString(amount, 'f', 2);
  return invariant + QLatin1Char(' ') + localized;
}

Operation* OperationFilterModel::operationAtInternal(int index) const {
  if (index < 0 || index >= rowCount())
    return nullptr;
  return data(this->index(index, 0), Account::OperationRole).value<Operation*>();
}

void OperationFilterModel::updateCurrentOperation() {
  if (_account && _account->currentOperation() && currentOperationIndex() < 0)
    _account->clearCurrentOperation();
  emit currentOperationChanged();
}
