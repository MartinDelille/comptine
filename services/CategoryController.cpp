#include <QCollator>
#include <QDate>
#include <QtMath>
#include <algorithm>

#include "BudgetData.h"
#include "CategoryController.h"
#include "UndoCommands.h"
#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"

using namespace Qt::StringLiterals;

bool isSameMonth(const QDate& d1, const QDate& d2) {
  return (d1.year() == d2.year()) && (d1.month() == d2.month());
}

CategoryController::CategoryController(BudgetData& budgetData,
                                       QUndoStack& undoStack) :
    _budgetData(budgetData),
    _undoStack(undoStack) {
  connect(&_budgetData, &BudgetData::operationDataChanged, this, &CategoryController::refresh);
  connect(&_budgetData, &BudgetData::operationDataChanged, this, &CategoryController::budgetDataChanged);
  connect(&_budgetData, &BudgetData::operationDataChanged, this, &CategoryController::evolutionDataChanged);
  connect(&_budgetData, &BudgetData::budgetDateChanged, this, &CategoryController::refresh);
  connect(&_budgetData, &BudgetData::budgetDateChanged, this, &CategoryController::budgetDataChanged);
  connect(this, &CategoryController::budgetDataChanged, this, &CategoryController::refresh);
  connect(this, &CategoryController::monthHistoryChanged, this, &CategoryController::refresh);
}

CategoryController::~CategoryController() {
  clear();
}

void CategoryController::clear() {
  if (_categories.isEmpty()) {
    return;
  }
  beginRemoveRows(QModelIndex(), 0, static_cast<int>(_categories.size()) - 1);
  for (auto* category : _categories) category->setParent(nullptr);
  qDeleteAll(_categories);
  _categories.clear();
  endRemoveRows();
  emit countChanged();
}

int CategoryController::currentIndex() const {
  return static_cast<int>(_categories.indexOf(_current));
}

void CategoryController::set_currentIndex(int index) {
  set_current(at(index));
}

int CategoryController::rowCount(const QModelIndex& parent) const {
  if (parent.isValid())
    return 0;

  return static_cast<int>(_categories.size());
}

int CategoryController::balancedCount() const {
  int result = 0;
  for (int index = 0; index < _categories.size(); ++index) {
    if (isBalanced(index)) {
      result++;
    }
  }

  return result;
}

bool CategoryController::isBalanced(int index) const {
  auto category = at(index);
  if (!category)
    return true;

  const auto date = _budgetData.budgetDate();
  const auto record = category->monthRecord(date);
  return qAbs(category->budgetLimitForMonth(date) - spentInCategory(category, date) + record.leftoverTotal()) < 0.01;
}

QVariant CategoryController::data(const QModelIndex& index, int role) const {
  if (!index.isValid())
    return QVariant();

  const int row = index.row();

  if (auto category = at(row)) {
    switch (static_cast<Roles>(role)) {
      case CategoryRole:
        return QVariant::fromValue(category);
      case AmountRole:
        return spentInCategory(category, _budgetData.budgetDate());
      case AccumulatedRole:
        return category->accumulatedLeftoverBefore(_budgetData.budgetDate());
      case LeftoverRole:
        return leftoverForCategory(category, _budgetData.budgetDate());
      case SaveAmountRole: {
        MonthRecord record = category->monthRecord(_budgetData.budgetDate());
        return record.saveAmount;
      }
      case ReportAmountRole: {
        MonthRecord record = category->monthRecord(_budgetData.budgetDate());
        return record.reportAmount;
      }
      case BudgetLimitRole:
        return category->budgetLimitForMonth(_budgetData.budgetDate());
    }
  }
  return QVariant();
}

QHash<int, QByteArray> CategoryController::roleNames() const {
  return {
    { CategoryRole, "category" },
    { AmountRole, "amount" },
    { AccumulatedRole, "accumulated" },
    { LeftoverRole, "leftover" },
    { SaveAmountRole, "saveAmount" },
    { ReportAmountRole, "reportAmount" },
    { BudgetLimitRole, "budgetLimit" },
  };
}

double CategoryController::totalIncome() const {
  double total = 0.0;
  for (auto category : _categories) {
    double budgetLimit = category->budgetLimitForMonth(_budgetData.budgetDate());
    if (budgetLimit > 0) {
      total += budgetLimit;
    }
  }
  return total;
}

double CategoryController::totalExpense() const {
  double total = 0.0;
  for (auto category : _categories) {
    double budgetLimit = category->budgetLimitForMonth(_budgetData.budgetDate());
    if (budgetLimit < 0) {
      total += -budgetLimit;  // Show expenses as positive
    }
  }
  return total;
}

double CategoryController::totalToSave() const {
  double total = 0.0;
  for (auto category : _categories) {
    MonthRecord record = category->monthRecord(_budgetData.budgetDate());
    total += record.saveAmount;
  }
  return total;
}

double CategoryController::totalToReport() const {
  double total = 0.0;
  for (auto category : _categories) {
    MonthRecord record = category->monthRecord(_budgetData.budgetDate());
    if (record.reportAmount > 0) {
      total += record.reportAmount;
    }
  }
  return total;
}

double CategoryController::totalFromReport() const {
  double total = 0.0;
  for (auto category : _categories) {
    MonthRecord record = category->monthRecord(_budgetData.budgetDate());
    if (record.reportAmount < 0) {
      total += -record.reportAmount;
    }
  }
  return total;
}

double CategoryController::netReport() const {
  return totalToReport() - totalFromReport();
}

QList<Category*> CategoryController::categories() const {
  return _categories;
}

Category* CategoryController::at(int index) const {
  if (index >= 0 && index < _categories.size()) {
    return _categories[index];
  }
  return nullptr;
}

int CategoryController::categoryIndex(const Category* category) const {
  return static_cast<int>(_categories.indexOf(const_cast<Category*>(category)));
}

Category* CategoryController::getCategoryByName(const QString& name) const {
  for (Category* category : _categories) {
    if (category->name() == name) {
      return category;
    }
  }
  return nullptr;
}

Category* CategoryController::addCategory(Category* category) {
  if (category == nullptr) {
    return nullptr;
  }
  // Skip if category with same name already exists
  if (getCategoryByName(category->name())) {
    delete category;
    return nullptr;
  }

  // Connect category signals so model refreshes when category data changes (e.g., via undo/redo)
  connect(category, &Category::monthHistoryChanged, this, &CategoryController::refresh);
  connect(category, &Category::monthHistoryChanged, this, &CategoryController::evolutionDataChanged);
  connect(category, &Category::nameChanged, this, &CategoryController::refresh);
  connect(category, &Category::nameChanged, this, &CategoryController::evolutionDataChanged);

  QCollator collator;
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  int insertRow = 0;
  while (insertRow < _categories.size() && collator.compare(_categories[insertRow]->name(), category->name()) < 0) {
    ++insertRow;
  }

  beginInsertRows(QModelIndex(), insertRow, insertRow);
  category->setParent(this);
  _categories.insert(insertRow, category);
  endInsertRows();
  emit countChanged();
  return category;
}

Category* CategoryController::takeCategoryByName(const QString& name) {
  for (int index = 0; index < _categories.size(); index++) {
    if (_categories[index]->name().compare(name, Qt::CaseInsensitive) == 0) {
      beginRemoveRows(QModelIndex(), index, index);
      auto cat = _categories.takeAt(index);
      cat->setParent(nullptr);
      endRemoveRows();
      emit countChanged();
      return cat;
    }
  }
  return nullptr;
}

QStringList CategoryController::categoryNames() const {
  QStringList names;
  for (auto category : _categories) {
    names.append(category->name());
  }
  return names;
}

double CategoryController::spentInCategory(const Category* category, const QDate& budgetDate) const {
  double total = 0.0;
  for (auto account : _budgetData.accounts()) {
    for (auto op : account->operations()) {
      if (isSameMonth(op->budgetDate(), budgetDate)) {
        // Use amountForCategory which handles both split and non-split operations
        total += op->amountForCategory(category);
      }
    }
  }
  return total;
}

QVariantList CategoryController::operationsForCategory(const Category* category, const QDate& date) const {
  QVariantList result;
  for (auto account : _budgetData.accounts()) {
    for (auto op : account->operations()) {
      if (isSameMonth(op->budgetDate(), date)) {
        // Check if this operation contributes to this category
        double categoryAmount = op->amountForCategory(category);
        if (!qFuzzyIsNull(categoryAmount)) {
          QVariantMap item;
          item["operation"_L1] = QVariant::fromValue(op);
          item["date"_L1] = op->date();
          item["budgetDate"_L1] = op->budgetDate();
          item["label"_L1] = op->label();
          item["amount"_L1] = categoryAmount;     // Show only the amount for this category
          item["totalAmount"_L1] = op->amount();  // Total operation amount
          item["isCategorized"_L1] = op->isCategorized();
          item["accountName"_L1] = account->name();
          result.append(item);
        }
      }
    }
  }

  // Sort by date (most recent first)
  std::sort(result.begin(), result.end(), [](const QVariant& a, const QVariant& b) {
    return a.toMap()["date"_L1].toDate() > b.toMap()["date"_L1].toDate();
  });

  return result;
}

double CategoryController::leftoverForCategory(const Category* category, const QDate& date) const {
  if (!category) return 0.0;

  double budgetLimit = category->budgetLimitForMonth(date);
  double spent = spentInCategory(category, date);

  // For expense categories (negative budget limit):
  // leftover = |budgetLimit| - |spent|
  // For income categories (positive budget limit):
  // leftover = received - expected

  bool isIncome = budgetLimit > 0;
  if (isIncome) {
    // Income: positive spent means income received
    // leftover = actual income - expected income
    return spent - budgetLimit;
  } else {
    // Expense: negative spent means money spent
    // leftover = budget - spent = -budgetLimit - (-spent)
    return -budgetLimit + spent;
  }
}

double CategoryController::accumulatedLeftover(const QString& categoryName, const QDate& date) const {
  auto category = getCategoryByName(categoryName);
  if (!category) return 0.0;
  return category->accumulatedLeftoverBefore(date);
}

void CategoryController::refresh() {
  emit dataChanged(
      index(0, 0),
      index(rowCount() - 1, 0));
}
