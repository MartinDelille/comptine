#include "Operation.h"

Operation::Operation(QObject* parent) :
    QObject(parent) {}

Operation::Operation(const QDate& date,
                     double amount,
                     const Category* category,
                     const QString& label,
                     const QString& details,
                     const QList<CategoryAllocation>& allocations,
                     QObject* parent) :
    QObject(parent),
    _date(date),
    _amount(amount),
    _category(category),
    _label(label),
    _details(details),
    _allocations(allocations) {
  auto updateCategoryName = [this] {
    if (_category) {
      connect(_category, &Category::nameChanged, this, &Operation::categoryChanged);
    }
  };
  connect(this, &Operation::categoryChanged, this, updateCategoryName);
  updateCategoryName();
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
    // Clear the single category when we have allocations
    if (!_allocations.isEmpty()) {
      _category = nullptr;
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
  QSet<QString> categoryNames;
  if (_allocations.isEmpty()) {
    return _category ? _category->name() : "";
  }

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
  if (_allocations.isEmpty()) {
    // Not split - return full amount if category matches
    if (_category == category) {
      return _amount;
    }
    return 0.0;
  }

  // Split - sum all allocations for this category
  double total = 0.0;
  for (const auto& alloc : _allocations) {
    if (alloc.category == category) {
      total += alloc.amount;
    }
  }
  return total;
}
