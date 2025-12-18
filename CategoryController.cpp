#include <QCollator>
#include <QDate>
#include <algorithm>

#include "Account.h"
#include "BudgetData.h"
#include "CategoryController.h"
#include "Operation.h"
#include "UndoCommands.h"

CategoryController::CategoryController(BudgetData& budgetData,
                                       QUndoStack& undoStack) :
    _budgetData(budgetData),
    _undoStack(undoStack),
    _leftoverModel(this) {
  _leftoverModel.setCategoryController(this);
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

void CategoryController::addCategory(const QString& name, double budgetLimit) {
  _undoStack.push(new AddCategoryCommand(this, new Category(name, budgetLimit)));
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
    // Sort by category name using locale-aware collation (handles accents properly)
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(_categories.begin(), _categories.end(), [&collator](const Category* a, const Category* b) {
      return collator.compare(a->name(), b->name()) < 0;
    });

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
  return names;
}

void CategoryController::editCategory(const QString& originalName, const QString& newName, double newBudgetLimit) {
  Category* category = getCategoryByName(originalName);
  if (!category) return;

  QString oldName = category->name();
  double oldBudgetLimit = category->budgetLimit();

  // Only create undo command if something changed
  if (oldName != newName || oldBudgetLimit != newBudgetLimit) {
    _undoStack.push(new EditCategoryCommand(*category, &_budgetData, this,
                                            oldName, newName,
                                            oldBudgetLimit, newBudgetLimit));
  }
}

double CategoryController::spentInCategory(const Category* category, int year, int month) const {
  double total = 0.0;
  for (const Account* account : _budgetData.accounts()) {
    for (const Operation* op : account->operations()) {
      // Use budgetDate for budget calculations (falls back to date if not set)
      QDate budgetDate = op->budgetDate();
      if (budgetDate.year() == year && budgetDate.month() == month) {
        // Use amountForCategory which handles both split and non-split operations
        total += op->amountForCategory(category);
      }
    }
  }
  return total;
}

QVariantList CategoryController::monthlyBudgetSummary(int year, int month) const {
  QVariantList result;
  for (const Category* category : _categories) {
    double total = spentInCategory(category, year, month);
    double budgetLimit = category->budgetLimit();
    bool isIncome = budgetLimit > 0;

    // Get accumulated leftover from previous months
    double accumulated = category->accumulatedLeftoverBefore(year, month);

    // For display: show positive values
    // - Expenses: total is negative, we show as positive "spent"
    // - Income: total is positive, we show as positive "received"
    double displayAmount = isIncome ? total : -total;
    double displayLimit = std::abs(budgetLimit);
    double remaining = displayLimit - displayAmount;

    // Calculate percent used based on effective limit:
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
    item["accumulated"] = accumulated;    // Accumulated leftover from previous months
    item["displayLimit"] = displayLimit;  // Budget
    result.append(item);
  }

  return result;
}

QVariantList CategoryController::operationsForCategory(const Category* category, int year, int month) const {
  QVariantList result;
  for (const Account* account : _budgetData.accounts()) {
    for (const Operation* op : account->operations()) {
      QDate budgetDate = op->budgetDate();
      if (budgetDate.year() == year && budgetDate.month() == month) {
        // Check if this operation contributes to this category
        double categoryAmount = op->amountForCategory(category);
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

double CategoryController::leftoverForCategory(const Category* category, int year, int month) const {
  if (!category) return 0.0;

  double budgetLimit = category->budgetLimit();
  double spent = spentInCategory(category, year, month);
  double accumulated = category->accumulatedLeftoverBefore(year, month);

  // For expense categories (negative budget limit):
  // leftover = |budgetLimit| - |spent| + accumulated
  // For income categories (positive budget limit):
  // leftover = received - expected + accumulated (extra income can be saved)

  bool isIncome = budgetLimit > 0;
  if (isIncome) {
    // Income: positive spent means income received
    // leftover = actual income - expected income + accumulated
    return spent - budgetLimit + accumulated;
  } else {
    // Expense: negative spent means money spent
    // leftover = budget - spent + accumulated = -budgetLimit - (-spent) + accumulated
    return -budgetLimit + spent + accumulated;
  }
}

double CategoryController::accumulatedLeftover(const QString& categoryName, int year, int month) const {
  Category* category = getCategoryByName(categoryName);
  if (!category) return 0.0;
  return category->accumulatedLeftoverBefore(year, month);
}

QVariantMap CategoryController::leftoverDecision(const QString& categoryName, int year, int month) const {
  Category* category = getCategoryByName(categoryName);
  QVariantMap result;
  result["saveAmount"] = 0.0;
  result["reportAmount"] = 0.0;

  if (!category) return result;

  LeftoverDecision decision = category->leftoverDecision(year, month);
  result["saveAmount"] = decision.saveAmount;
  result["reportAmount"] = decision.reportAmount;
  return result;
}

QVariantList CategoryController::leftoverSummary(int year, int month) const {
  QVariantList result;

  for (const Category* category : _categories) {
    double budgetLimit = category->budgetLimit();
    double spent = spentInCategory(category, year, month);
    double accumulated = category->accumulatedLeftoverBefore(year, month);
    bool isIncome = budgetLimit > 0;

    // Calculate leftover
    double leftover;
    if (isIncome) {
      leftover = spent - budgetLimit;
    } else {
      leftover = -budgetLimit + spent;
    }

    // Get existing decision for this month
    LeftoverDecision decision = category->leftoverDecision(year, month);

    QVariantMap item;
    item["name"] = category->name();
    item["budgetLimit"] = std::abs(budgetLimit);
    item["spent"] = isIncome ? spent : -spent;  // Show as positive
    item["accumulated"] = accumulated;
    item["leftover"] = leftover;
    item["isIncome"] = isIncome;

    // Decision amounts
    item["saveAmount"] = decision.saveAmount;
    item["reportAmount"] = decision.reportAmount;
    qDebug() << leftover << category->name();

    result.append(item);
  }

  return result;
}

QVariantMap CategoryController::leftoverTotals(int year, int month) const {
  double totalToSave = 0.0;
  double totalToReport = 0.0;
  double totalFromReport = 0.0;  // Negative leftovers drawn from accumulated

  for (const Category* category : _categories) {
    LeftoverDecision decision = category->leftoverDecision(year, month);
    totalToSave += decision.saveAmount;
    if (decision.reportAmount > 0) {
      totalToReport += decision.reportAmount;
    } else if (decision.reportAmount < 0) {
      totalFromReport += -decision.reportAmount;  // Track as positive "from" amount
    }
  }

  QVariantMap result;
  result["toSave"] = totalToSave;
  result["toReport"] = totalToReport;
  result["fromReport"] = totalFromReport;
  result["netReport"] = totalToReport - totalFromReport;
  return result;
}

void CategoryController::setLeftoverAmounts(const QString& categoryName, int year, int month,
                                            double saveAmount, double reportAmount) {
  Category* category = getCategoryByName(categoryName);
  if (!category) return;

  LeftoverDecision oldDecision = category->leftoverDecision(year, month);
  LeftoverDecision newDecision{ saveAmount, reportAmount };

  // Only create undo command if something changed
  if (!qFuzzyCompare(oldDecision.saveAmount, newDecision.saveAmount) || !qFuzzyCompare(oldDecision.reportAmount, newDecision.reportAmount)) {
    _undoStack.push(new SetLeftoverDecisionCommand(*category, this, year, month, oldDecision, newDecision));
  }
}
