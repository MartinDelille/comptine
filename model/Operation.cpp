#include "Operation.h"
#include "Account.h"

Operation::Operation(Account* account,
                     const QDate& date,
                     double amount,
                     const QString& label,
                     const QString& details,
                     const QList<Allocation*>& allocations) :
    _account(account),
    _date(date),
    _amount(amount),
    _label(label),
    _details(details),
    _allocations(allocations) {
  for (auto* allocation : _allocations) {
    if (!allocation) continue;
    allocation->setParent(this);
  }
  connectCategorySignals(_allocations);
}

void Operation::connectCategorySignals(const QList<Allocation*>& allocations) {
  for (auto* allocation : allocations) {
    if (allocation && allocation->category()) {
      connect(allocation->category(), &Category::nameChanged,
              this, &Operation::allocationsChanged, Qt::UniqueConnection);
    }
  }
}

void Operation::disconnectCategorySignals(const QList<Allocation*>& allocations) {
  for (auto* allocation : allocations) {
    if (allocation && allocation->category()) {
      disconnect(allocation->category(), &Category::nameChanged,
                 this, &Operation::allocationsChanged);
    }
  }
}

int Operation::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : _allocations.size();
}

QVariant Operation::data(const QModelIndex& modelIndex, int role) const {
  if (!modelIndex.isValid() || modelIndex.row() < 0 || modelIndex.row() >= _allocations.size())
    return {};
  const auto* allocation = _allocations.at(modelIndex.row());
  if (!allocation) return {};
  switch (role) {
    case CategoryRole:
      return QVariant::fromValue(allocation->category());
    case AmountRole:
      return allocation->amount();
  }
  return {};
}

QHash<int, QByteArray> Operation::roleNames() const {
  return { { CategoryRole, "category" }, { AmountRole, "amount" } };
}

Operation::~Operation() {
  clearAllocations();
}

QDate Operation::budgetDate() const {
  // Return explicit budget date if set, otherwise fall back to operation date
  return _budgetDate.isValid() ? _budgetDate : _date;
}

void Operation::set_budgetDate(QDate value) {
  if (_budgetDate != value) {
    _budgetDate = value;
    emit budgetDateChanged();
  }
}

QStringList Operation::allocatedCategoryNames() const {
  QStringList names;
  for (const auto& alloc : _allocations) {
    if (alloc && alloc->category()) {
      names.append(alloc->category()->name());
    }
  }
  return names;
}

void Operation::setAllocations(const QList<Allocation*>& allocations) {
  if (!sameAllocations(allocations)) {
    disconnectCategorySignals(_allocations);
    beginResetModel();
    qDeleteAll(_allocations);
    _allocations = allocations;
    for (auto* allocation : _allocations) {
      if (allocation) allocation->setParent(this);
    }
    endResetModel();
    connectCategorySignals(_allocations);
    emit allocationsChanged();
  }
}

void Operation::clearAllocations() {
  if (!_allocations.isEmpty()) {
    disconnectCategorySignals(_allocations);
    beginResetModel();
    qDeleteAll(_allocations);
    _allocations.clear();
    endResetModel();
    emit allocationsChanged();
  }
}

bool Operation::sameAllocations(const QList<Allocation*>& otherAllocations) const {
  if (_allocations.count() != otherAllocations.count()) {
    return false;
  }
  for (int i = 0; i < _allocations.count(); i++) {
    if (_allocations[i] == nullptr || otherAllocations[i] == nullptr) {
      if (_allocations[i] != otherAllocations[i]) return false;
      continue;
    }
    if (*_allocations[i] != *otherAllocations[i]) {
      return false;
    }
  }
  return true;
}

bool Operation::isCategorized() const {
  double totalAmount = 0.0;
  for (auto allocation : _allocations) {
    if (allocation) totalAmount += allocation->amount();
  }
  return qFuzzyCompare(totalAmount, _amount);
}

double Operation::allocatedAmount() const {
  double totalAmount = 0.0;
  for (auto* allocation : _allocations) {
    if (allocation) totalAmount += allocation->amount();
  }
  return totalAmount;
}

QString Operation::categoryDisplay() const {
  QSet<QString> categoryNames;

  // Return comma-separated list of categories
  QSet<const Category*> uniqueCategories;
  for (const auto& alloc : _allocations) {
    if (alloc && alloc->category() && !uniqueCategories.contains(alloc->category())) {
      uniqueCategories.insert(alloc->category());
    }
  }
  QStringList displayNames;
  for (auto category : uniqueCategories) {
    displayNames.append(category->name());
  }
  return displayNames.join(", ");
}

double Operation::amountForCategory(const Category* category) const {
  // Split - sum all allocations for this category
  double total = 0.0;
  for (const auto& alloc : _allocations) {
    if (alloc && alloc->category() && alloc->category() == category) {
      total += alloc->amount();
    }
  }
  return total;
}
