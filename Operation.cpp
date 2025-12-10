#include "Operation.h"

Operation::Operation(QObject *parent) :
    QObject(parent) {}

Operation::Operation(const QDate &date, double amount, const QString &category,
                     const QString &description, QObject *parent) :
    QObject(parent), _date(date), _amount(amount), _category(category), _description(description) {}

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

QVariantList Operation::allocations() const {
  QVariantList result;
  for (const auto &alloc : _allocations) {
    QVariantMap item;
    item["category"] = alloc.category;
    item["amount"] = alloc.amount;
    result.append(item);
  }
  return result;
}

void Operation::setAllocations(const QList<CategoryAllocation> &allocations) {
  if (_allocations != allocations) {
    _allocations = allocations;
    // Clear the single category when we have allocations
    if (!_allocations.isEmpty()) {
      _category.clear();
    }
    emit allocationsChanged();
    emit categoryChanged();  // categoryDisplay may have changed
  }
}

void Operation::clearAllocations() {
  if (!_allocations.isEmpty()) {
    _allocations.clear();
    emit allocationsChanged();
  }
}

bool Operation::isSplit() const {
  return !_allocations.isEmpty();
}

QString Operation::categoryDisplay() const {
  if (_allocations.isEmpty()) {
    return _category;
  }

  // Return comma-separated list of categories
  QStringList categories;
  for (const auto &alloc : _allocations) {
    if (!categories.contains(alloc.category)) {
      categories.append(alloc.category);
    }
  }
  return categories.join(", ");
}

double Operation::amountForCategory(const QString &categoryName) const {
  if (_allocations.isEmpty()) {
    // Not split - return full amount if category matches
    return (_category == categoryName) ? _amount : 0.0;
  }

  // Split - sum all allocations for this category
  double total = 0.0;
  for (const auto &alloc : _allocations) {
    if (alloc.category == categoryName) {
      total += alloc.amount;
    }
  }
  return total;
}
