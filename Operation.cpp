#include "Operation.h"
#include "Account.h"

Operation::Operation(Account* account,
                     const QDate& date,
                     double amount,
                     const QString& label,
                     const QString& details,
                     const QList<Allocation*>& allocations) :
    QObject(account),
    _account(account),
    _date(date),
    _amount(amount),
    _label(label),
    _details(details),
    _allocations(allocations) {
  for (auto alloc : _allocations)
    alloc->setParent(this);
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

void Operation::setAllocations(const QList<Allocation*>& allocations) {
  if (!sameAllocations(allocations)) {
    _allocations = allocations;
    for (auto alloc : _allocations)
      alloc->setParent(this);
    emit allocationsChanged();
  }
}

void Operation::clearAllocations() {
  if (!_allocations.isEmpty()) {
    qDeleteAll(_allocations);
    _allocations.clear();
    emit allocationsChanged();
  }
}

bool Operation::sameAllocations(const QList<Allocation*>& otherAllocations) const {
  if (_allocations.count() != otherAllocations.count()) {
    return false;
  }
  for (int i = 0; i < _allocations.count(); i++) {
    if (*_allocations[i] != *otherAllocations[i]) {
      return false;
    }
  }
  return true;
}

bool Operation::isCategorized() const {
  double totalAmount = 0.0;
  for (auto allocation : _allocations) {
    totalAmount += allocation->amount();
  }
  return totalAmount == _amount;
}

QString Operation::categoryDisplay() const {
  QSet<QString> categoryNames;

  // Return comma-separated list of categories
  QSet<const Category*> uniqueCategories;
  for (const auto& alloc : _allocations) {
    if (!uniqueCategories.contains(alloc->category())) {
      uniqueCategories.insert(alloc->category());
    }
  }
  QStringList displayNames;
  for (const Category* category : uniqueCategories) {
    displayNames.append(category->name());
  }
  return displayNames.join(", ");
}

double Operation::amountForCategory(const Category* category) const {
  // Split - sum all allocations for this category
  double total = 0.0;
  for (const auto& alloc : _allocations) {
    if (alloc->category() == category) {
      total += alloc->amount();
    }
  }
  return total;
}
