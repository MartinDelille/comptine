#include "Operation.h"

Operation::Operation(QObject* parent) :
    QObject(parent) {}

Operation::Operation(const QDate& date,
                     double amount,
                     const QString& label,
                     const QString& details,
                     const QList<CategoryAllocation>& allocations,
                     QObject* parent) :
    QObject(parent),
    _date(date),
    _amount(amount),
    _label(label),
    _details(details),
    _allocations(allocations) {
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

QVariantList Operation::allocations() const {
  QVariantList result;
  for (const auto& alloc : _allocations) {
    QVariantMap item;
    item["category"] = alloc.category ? alloc.category->name() : QVariant();
    item["amount"] = alloc.amount;
    result.append(item);
  }
  return result;
}

void Operation::setAllocations(const QList<CategoryAllocation>& allocations) {
  if (_allocations != allocations) {
    _allocations = allocations;
    emit allocationsChanged();
  }
}

void Operation::clearAllocations() {
  if (!_allocations.isEmpty()) {
    _allocations.clear();
    emit allocationsChanged();
  }
}

bool Operation::isCategorized() const {
  double totalAmount = 0.0;
  for (auto allocation : _allocations) {
    totalAmount += allocation.amount;
  }
  return totalAmount == _amount;
}

QString Operation::categoryDisplay() const {
  QSet<QString> categoryNames;

  // Return comma-separated list of categories
  QSet<const Category*> uniqueCategories;
  for (const auto& alloc : _allocations) {
    if (!uniqueCategories.contains(alloc.category)) {
      uniqueCategories.insert(alloc.category);
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
    if (alloc.category == category) {
      total += alloc.amount;
    }
  }
  return total;
}
