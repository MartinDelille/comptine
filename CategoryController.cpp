#include <QDate>
#include <algorithm>

#include "Account.h"
#include "BudgetData.h"
#include "CategoryController.h"
#include "Operation.h"
#include "UndoCommands.h"

CategoryController::CategoryController(QObject* parent) : QObject(parent) {
}

void CategoryController::setBudgetData(BudgetData* budgetData) {
  _budgetData = budgetData;
}

void CategoryController::setUndoStack(QUndoStack* undoStack) {
  _undoStack = undoStack;
}

int CategoryController::categoryCount() const {
  return _categories.size();
}

QList<Category*> CategoryController::categories() const {
  return _categories;
}

Category* CategoryController::getCategory(int index) const {
  if (index >= 0 && index < _categories.size()) {
    return _categories[index];
  }
  return nullptr;
}

Category* CategoryController::getCategoryByName(const QString& name) const {
  for (Category* category : _categories) {
    if (category->name() == name) {
      return category;
    }
  }
  return nullptr;
}

void CategoryController::addCategory(Category* category) {
  if (category) {
    // Skip if category with same name already exists
    if (getCategoryByName(category->name())) {
      delete category;
      return;
    }
    category->setParent(this);
    _categories.append(category);
    emit categoryCountChanged();
  }
}

void CategoryController::removeCategory(int index) {
  if (index >= 0 && index < _categories.size()) {
    delete _categories.takeAt(index);
    emit categoryCountChanged();
  }
}

void CategoryController::clearCategories() {
  qDeleteAll(_categories);
  _categories.clear();
  emit categoryCountChanged();
}

Category* CategoryController::takeCategoryByName(const QString& name) {
  for (int i = 0; i < _categories.size(); i++) {
    if (_categories[i]->name().compare(name, Qt::CaseInsensitive) == 0) {
      Category* cat = _categories.takeAt(i);
      cat->setParent(nullptr);  // Release Qt ownership
      emit categoryCountChanged();
      return cat;
    }
  }
  return nullptr;
}

QStringList CategoryController::categoryNames() const {
  QStringList names;
  for (const Category* category : _categories) {
    names.append(category->name());
  }
  names.sort(Qt::CaseInsensitive);
  return names;
}

void CategoryController::editCategory(const QString& originalName, const QString& newName, double newBudgetLimit) {
  if (!_undoStack) return;

  Category* category = getCategoryByName(originalName);
  if (!category) return;

  QString oldName = category->name();
  double oldBudgetLimit = category->budgetLimit();

  // Only create undo command if something changed
  if (oldName != newName || oldBudgetLimit != newBudgetLimit) {
    _undoStack->push(new EditCategoryCommand(category, _budgetData, this,
                                             oldName, newName,
                                             oldBudgetLimit, newBudgetLimit));
  }
}

double CategoryController::spentInCategory(const QString& categoryName, int year, int month) const {
  if (!_budgetData) return 0.0;

  double total = 0.0;
  for (const Account* account : _budgetData->accounts()) {
    for (const Operation* op : account->operations()) {
      // Use budgetDate for budget calculations (falls back to date if not set)
      QDate budgetDate = op->budgetDate();
      if (budgetDate.year() == year && budgetDate.month() == month) {
        // Use amountForCategory which handles both split and non-split operations
        total += op->amountForCategory(categoryName);
      }
    }
  }
  return total;
}

QVariantList CategoryController::monthlyBudgetSummary(int year, int month) const {
  QVariantList result;
  for (const Category* category : _categories) {
    double total = spentInCategory(category->name(), year, month);
    double budgetLimit = category->budgetLimit();
    bool isIncome = budgetLimit > 0;

    // For display: show positive values
    // - Expenses: total is negative, we show as positive "spent"
    // - Income: total is positive, we show as positive "received"
    double displayAmount = isIncome ? total : -total;
    double displayLimit = std::abs(budgetLimit);
    double remaining = displayLimit - displayAmount;

    // Calculate percent used:
    // - For zero budget expense: any spending means exceeded (use infinity-like behavior)
    // - For zero budget income: no expectation yet
    double percentUsed;
    if (displayLimit > 0) {
      percentUsed = (displayAmount / displayLimit) * 100.0;
    } else if (!isIncome && displayAmount > 0) {
      // Zero expense budget with spending = exceeded (show as 100%+)
      percentUsed = 100.0 + displayAmount;  // Will show exceeded status
    } else {
      percentUsed = 0.0;
    }

    QVariantMap item;
    item["name"] = category->name();
    item["budgetLimit"] = displayLimit;
    item["signedBudgetLimit"] = budgetLimit;  // Original signed value for editing
    item["amount"] = displayAmount;
    item["remaining"] = remaining;
    item["percentUsed"] = percentUsed;
    item["isIncome"] = isIncome;
    result.append(item);
  }

  // Sort by category name
  std::sort(result.begin(), result.end(), [](const QVariant& a, const QVariant& b) {
    return a.toMap()["name"].toString().compare(b.toMap()["name"].toString(), Qt::CaseInsensitive) < 0;
  });

  return result;
}

QVariantList CategoryController::operationsForCategory(const QString& categoryName, int year, int month) const {
  if (!_budgetData) return {};

  QVariantList result;
  for (const Account* account : _budgetData->accounts()) {
    for (const Operation* op : account->operations()) {
      QDate budgetDate = op->budgetDate();
      if (budgetDate.year() == year && budgetDate.month() == month) {
        // Check if this operation contributes to this category
        double categoryAmount = op->amountForCategory(categoryName);
        if (!qFuzzyIsNull(categoryAmount)) {
          QVariantMap item;
          item["date"] = op->date();
          item["budgetDate"] = budgetDate;
          item["description"] = op->description();
          item["amount"] = categoryAmount;     // Show only the amount for this category
          item["totalAmount"] = op->amount();  // Total operation amount
          item["isSplit"] = op->isSplit();
          item["accountName"] = account->name();
          result.append(item);
        }
      }
    }
  }

  // Sort by date (most recent first)
  std::sort(result.begin(), result.end(), [](const QVariant& a, const QVariant& b) {
    return a.toMap()["date"].toDate() > b.toMap()["date"].toDate();
  });

  return result;
}
