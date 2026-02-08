#include <QCollator>
#include <QDate>
#include <algorithm>

#include "Account.h"
#include "BudgetData.h"
#include "CategoryController.h"
#include "Operation.h"
#include "UndoCommands.h"

bool isSameMonth(const QDate& d1, const QDate& d2) {
  return (d1.year() == d2.year()) && (d1.month() == d2.month());
}

CategoryController::CategoryController(BudgetData& budgetData,
                                       const NavigationController& navigation,
                                       QUndoStack& undoStack) :
    _budgetData(budgetData),
    _navigation(navigation),
    _undoStack(undoStack) {
  connect(&_budgetData, &BudgetData::operationDataChanged, this, &CategoryController::refresh);
  connect(&_budgetData, &BudgetData::operationDataChanged, this, &CategoryController::leftoverDataChanged);
  connect(&_navigation, &NavigationController::currentCategoryIndexChanged, this, &CategoryController::currentChanged);
  connect(&_navigation, &NavigationController::budgetDateChanged, this, &CategoryController::refresh);
  connect(&_navigation, &NavigationController::budgetDateChanged, this, &CategoryController::leftoverDataChanged);
  connect(this, &CategoryController::leftoverDataChanged, this, &CategoryController::refresh);
  connect(this, &CategoryController::monthHistoryChanged, this, &CategoryController::refresh);
}

Category* CategoryController::current() const {
  return at(_navigation.currentCategoryIndex());
}

int CategoryController::rowCount(const QModelIndex& parent) const {
  if (parent.isValid())
    return 0;

  return _categories.size();
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
        return spentInCategory(category, _navigation.budgetDate());
      case AccumulatedRole:
        return category->accumulatedLeftoverBefore(_navigation.budgetDate());
      case LeftoverRole:
        return leftoverForCategory(category, _navigation.budgetDate());
      case SaveAmountRole: {
        MonthRecord record = category->monthRecord(_navigation.budgetDate().year(), _navigation.budgetDate().month());
        return record.saveAmount;
      }
      case ReportAmountRole: {
        MonthRecord record = category->monthRecord(_navigation.budgetDate().year(), _navigation.budgetDate().month());
        return record.reportAmount;
      }
      case BudgetLimitRole:
        return category->budgetLimitForMonth(_navigation.budgetDate());
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

double CategoryController::totalToSave() const {
  double total = 0.0;
  for (const Category* category : _categories) {
    MonthRecord record = category->monthRecord(_navigation.budgetDate().year(), _navigation.budgetDate().month());
    total += record.saveAmount;
  }
  return total;
}

double CategoryController::totalToReport() const {
  double total = 0.0;
  for (const Category* category : _categories) {
    MonthRecord record = category->monthRecord(_navigation.budgetDate().year(), _navigation.budgetDate().month());
    if (record.reportAmount > 0) {
      total += record.reportAmount;
    }
  }
  return total;
}

double CategoryController::totalFromReport() const {
  double total = 0.0;
  for (const Category* category : _categories) {
    MonthRecord record = category->monthRecord(_navigation.budgetDate().year(), _navigation.budgetDate().month());
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

Category* CategoryController::getCategoryByName(const QString& name) const {
  for (Category* category : _categories) {
    if (category->name() == name) {
      return category;
    }
  }
  return nullptr;
}

Category* CategoryController::addCategory(const QString& name, double budgetLimit) {
  auto category = new Category(name, budgetLimit);
  _undoStack.push(new AddCategoryCommand(this, category));
  return category;
}

void CategoryController::addCategory(Category* category) {
  if (category == nullptr) {
    return;
  }
  // Skip if category with same name already exists
  if (getCategoryByName(category->name())) {
    delete category;
    return;
  }
  category->setParent(this);

  // Connect category signals so model refreshes when category data changes (e.g., via undo/redo)
  connect(category, &Category::budgetLimitChanged, this, &CategoryController::refresh);
  connect(category, &Category::monthHistoryChanged, this, &CategoryController::refresh);
  connect(category, &Category::nameChanged, this, &CategoryController::refresh);

  QCollator collator;
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  int insertRow = 0;
  while (insertRow < _categories.size() && collator.compare(_categories[insertRow]->name(), category->name()) < 0) {
    ++insertRow;
  }

  beginInsertRows(QModelIndex(), insertRow, insertRow);
  _categories.insert(insertRow, category);
  endInsertRows();
  emit countChanged();
}

void CategoryController::removeCategory(int index) {
  if (index >= 0 && index < _categories.size()) {
    beginRemoveRows(QModelIndex(), index, index);
    delete _categories.takeAt(index);
    endRemoveRows();
    emit countChanged();
  }
}

void CategoryController::clear() {
  beginRemoveRows(QModelIndex(), 0, _categories.size() - 1);
  endRemoveRows();
  qDeleteAll(_categories);
  _categories.clear();
  emit countChanged();
}

Category* CategoryController::takeCategoryByName(const QString& name) {
  for (int index = 0; index < _categories.size(); index++) {
    if (_categories[index]->name().compare(name, Qt::CaseInsensitive) == 0) {
      beginRemoveRows(QModelIndex(), index, index);
      Category* cat = _categories.takeAt(index);
      cat->setParent(nullptr);  // Release Qt ownership
      endRemoveRows();
      emit countChanged();
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

void CategoryController::editCategory(const QString& originalName, const QString& newName, double newBudgetLimit, const QDate& budgetDate) {
  Category* category = getCategoryByName(originalName);
  if (!category) return;

  QString oldName = category->name();
  double oldBudgetLimit = category->budgetLimit();

  // Only create undo command if something changed
  if (oldName != newName || oldBudgetLimit != newBudgetLimit) {
    _undoStack.push(new EditCategoryCommand(*category,
                                            oldName, newName,
                                            oldBudgetLimit, newBudgetLimit,
                                            budgetDate));
  }
}

double CategoryController::spentInCategory(const Category* category, const QDate& budgetDate) const {
  double total = 0.0;
  for (const Account* account : _budgetData.accounts()) {
    for (const Operation* op : account->operations()) {
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
  for (const Account* account : _budgetData.accounts()) {
    for (const Operation* op : account->operations()) {
      if (isSameMonth(op->budgetDate(), date)) {
        // Check if this operation contributes to this category
        double categoryAmount = op->amountForCategory(category);
        if (!qFuzzyIsNull(categoryAmount)) {
          QVariantMap item;
          item["operation"] = QVariant::fromValue(op);
          item["date"] = op->date();
          item["budgetDate"] = op->budgetDate();
          item["label"] = op->label();
          item["amount"] = categoryAmount;     // Show only the amount for this category
          item["totalAmount"] = op->amount();  // Total operation amount
          item["isCategorized"] = op->isCategorized();
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
  Category* category = getCategoryByName(categoryName);
  if (!category) return 0.0;
  return category->accumulatedLeftoverBefore(date);
}

QVariantMap CategoryController::leftoverDecision(const QString& categoryName, const QDate& date) const {
  Category* category = getCategoryByName(categoryName);
  QVariantMap result;
  result["saveAmount"] = 0.0;
  result["reportAmount"] = 0.0;

  if (!category) return result;

  LeftoverDecision decision = category->monthRecord(date.year(), date.month());
  result["saveAmount"] = decision.saveAmount;
  result["reportAmount"] = decision.reportAmount;
  return result;
}

QVariantList CategoryController::leftoverSummary(const QDate& date) const {
  QVariantList result;

  for (const Category* category : _categories) {
    double budgetLimit = category->budgetLimitForMonth(date);
    double spent = spentInCategory(category, date);
    double accumulated = category->accumulatedLeftoverBefore(date);
    bool isIncome = budgetLimit > 0;

    // Calculate leftover
    double leftover;
    if (isIncome) {
      leftover = spent - budgetLimit;
    } else {
      leftover = -budgetLimit + spent;
    }

    // Get existing decision for this month
    MonthRecord record = category->monthRecord(date.year(), date.month());

    QVariantMap item;
    item["name"] = category->name();
    item["budgetLimit"] = std::abs(budgetLimit);
    item["spent"] = isIncome ? spent : -spent;  // Show as positive
    item["accumulated"] = accumulated;
    item["leftover"] = leftover;
    item["isIncome"] = isIncome;

    // Decision amounts
    item["saveAmount"] = record.saveAmount;
    item["reportAmount"] = record.reportAmount;

    result.append(item);
  }

  return result;
}

QVariantMap CategoryController::leftoverTotals(const QDate& date) const {
  double totalToSave = 0.0;
  double totalToReport = 0.0;
  double totalFromReport = 0.0;  // Negative leftovers drawn from accumulated

  for (const Category* category : _categories) {
    MonthRecord record = category->monthRecord(date.year(), date.month());
    totalToSave += record.saveAmount;
    if (record.reportAmount > 0) {
      totalToReport += record.reportAmount;
    } else if (record.reportAmount < 0) {
      totalFromReport += -record.reportAmount;  // Track as positive "from" amount
    }
  }

  QVariantMap result;
  result["toSave"] = totalToSave;
  result["toReport"] = totalToReport;
  result["fromReport"] = totalFromReport;
  result["netReport"] = totalToReport - totalFromReport;
  return result;
}

void CategoryController::setLeftoverAmounts(const QString& categoryName,
                                            const QDate& date,
                                            double saveAmount, double reportAmount) {
  Category* category = getCategoryByName(categoryName);
  if (!category) return;

  MonthRecord oldRecord = category->monthRecord(date.year(), date.month());
  MonthRecord newRecord = oldRecord;  // Preserve budgetLimit if set
  newRecord.saveAmount = saveAmount;
  newRecord.reportAmount = reportAmount;

  // Only create undo command if something changed
  if (!qFuzzyCompare(oldRecord.saveAmount, newRecord.saveAmount) || !qFuzzyCompare(oldRecord.reportAmount, newRecord.reportAmount)) {
    _undoStack.push(new SetLeftoverDecisionCommand(*category, this, date, oldRecord, newRecord));
  }
}

void CategoryController::refresh() {
  emit dataChanged(
      index(0, 0),
      index(rowCount() - 1, 0));
}
