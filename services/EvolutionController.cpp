#include "EvolutionController.h"

#include "BudgetData.h"
#include "CategoryController.h"
#include "model/Operation.h"

EvolutionController::EvolutionController(BudgetData& budgetData,
                                         CategoryController& categories,
                                         QObject* parent) :
    QAbstractTableModel(parent), _budgetData(budgetData), _categories(categories) {
  connect(&_budgetData, &BudgetData::budgetDateChanged, this,
          &EvolutionController::updateAvailableMonths);
  connect(&_budgetData, &BudgetData::operationDataChanged, this,
          &EvolutionController::updateAvailableMonths);
  connect(&_categories, &CategoryController::countChanged, this,
          &EvolutionController::updateAvailableMonths);
  connect(&_categories, &QAbstractItemModel::dataChanged, this,
          &EvolutionController::updateAvailableMonths);
  connect(&_categories, &CategoryController::budgetDataChanged, this,
          &EvolutionController::refreshData);
  connect(&_categories, &CategoryController::monthHistoryChanged, this,
          &EvolutionController::updateAvailableMonths);
  connect(&_categories, &CategoryController::currentChanged, this,
          &EvolutionController::refreshSelection);
  updateAvailableMonths();
}

QDate EvolutionController::monthDate(int column) const {
  return _availableMonths.value(column);
}

int EvolutionController::monthCount() const {
  return _availableMonths.size();
}

int EvolutionController::currentMonthIndex() const {
  const QDate selected(_budgetData.budgetDate().year(), _budgetData.budgetDate().month(), 1);
  return _availableMonths.indexOf(selected);
}

QDate EvolutionController::firstMonth() const {
  return _availableMonths.value(0);
}

QDate EvolutionController::lastMonth() const {
  return _availableMonths.isEmpty() ? QDate() : _availableMonths.constLast();
}

QDate EvolutionController::historyStart() const {
  QDate result;
  for (const Account* account : _budgetData.accounts()) {
    for (const Operation* operation : account->operations()) {
      const QDate month(operation->budgetDate().year(), operation->budgetDate().month(), 1);
      if (month.isValid() && (!result.isValid() || month < result)) {
        result = month;
      }
    }
  }
  for (const Category* category : _categories.categories()) {
    const auto history = category->allMonthHistory();
    for (auto it = history.constBegin(); it != history.constEnd(); ++it) {
      const QDate month(it.key().year, it.key().month, 1);
      if (month.isValid() && (!result.isValid() || month < result)) {
        result = month;
      }
    }
  }
  return result;
}

QDate EvolutionController::historyEnd() const {
  QDate result;
  for (const Account* account : _budgetData.accounts()) {
    for (const Operation* operation : account->operations()) {
      const QDate month(operation->budgetDate().year(), operation->budgetDate().month(), 1);
      if (month.isValid() && (!result.isValid() || month > result)) {
        result = month;
      }
    }
  }
  for (const Category* category : _categories.categories()) {
    const auto history = category->allMonthHistory();
    for (auto it = history.constBegin(); it != history.constEnd(); ++it) {
      const QDate month(it.key().year, it.key().month, 1);
      if (month.isValid() && (!result.isValid() || month > result)) {
        result = month;
      }
    }
  }
  return result;
}

QList<QDate> EvolutionController::calculateAvailableMonths() const {
  const QDate selected(_budgetData.budgetDate().year(), _budgetData.budgetDate().month(), 1);
  QDate start = historyStart();
  QDate end = historyEnd();
  if (!start.isValid()) return { selected };

  start = std::min(start, selected);
  end = std::max(end, selected);

  QList<QDate> result;
  for (QDate month = start; month <= end; month = month.addMonths(1)) {
    result.append(month);
  }
  return result;
}

void EvolutionController::updateAvailableMonths() {
  const QList<QDate> months = calculateAvailableMonths();
  if (months != _availableMonths) {
    _availableMonths = months;
    emit availableMonthsChanged();
    emit currentMonthIndexChanged();
    emit firstMonthChanged();
    emit lastMonthChanged();
    resetModel();
  } else {
    emit currentMonthIndexChanged();
    refreshData();
  }
}

int EvolutionController::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : _categories.categories().size();
}

int EvolutionController::columnCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : _availableMonths.size();
}

QVariant EvolutionController::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() >= _categories.categories().size()) {
    return {};
  }

  auto* category = _categories.categories().at(index.row());
  const QDate month = monthDate(index.column());
  switch (static_cast<Roles>(role)) {
    case DisplayRole:
      return category->budgetLimitForMonth(month);
    case CategoryNameRole:
      return category->name();
    case MonthDateRole:
      return month;
    case BudgetRole:
      return category->budgetLimitForMonth(month);
    case SpentRole:
      return _categories.spentInCategory(category, month);
    case LeftoverRole:
      return _categories.leftoverForCategory(category, month);
    case SavedRole:
      return category->monthRecord(month.year(), month.month()).saveAmount;
    case ReportedRole:
      return category->monthRecord(month.year(), month.month()).reportAmount;
    case AccumulatedRole:
      return category->accumulatedLeftoverBefore(month);
    case CurrentMonthRole: {
      const QDate selected(_budgetData.budgetDate().year(), _budgetData.budgetDate().month(), 1);
      return month == selected;
    }
    case CurrentCategoryRole:
      return index.row() == _categories.currentIndex();
  }
  return {};
}

QVariant EvolutionController::headerData(int section, Qt::Orientation orientation,
                                         int role) const {
  if (section < 0 || (orientation == Qt::Horizontal && section >= columnCount()) || (orientation == Qt::Vertical && section >= rowCount())) {
    return {};
  }
  if (orientation == Qt::Horizontal) {
    const QDate month = monthDate(section);
    if (role == DisplayRole || role == MonthDateRole) return month;
    if (role == CurrentMonthRole) {
      const QDate selected(_budgetData.budgetDate().year(), _budgetData.budgetDate().month(), 1);
      return month == selected;
    }
    return {};
  }
  if (orientation == Qt::Vertical) {
    if (role == DisplayRole || role == CategoryNameRole) {
      return _categories.categories().at(section)->name();
    }
    if (role == CurrentCategoryRole) {
      return section == _categories.currentIndex();
    }
  }
  return {};
}

QHash<int, QByteArray> EvolutionController::roleNames() const {
  return {
    { DisplayRole, "display" },
    { CategoryNameRole, "categoryName" },
    { MonthDateRole, "monthDate" },
    { BudgetRole, "budget" },
    { SpentRole, "spent" },
    { LeftoverRole, "leftover" },
    { SavedRole, "saved" },
    { ReportedRole, "reported" },
    { AccumulatedRole, "accumulated" },
    { CurrentMonthRole, "currentMonth" },
    { CurrentCategoryRole, "currentCategory" },
  };
}

void EvolutionController::resetModel() {
  beginResetModel();
  endResetModel();
  emit countChanged();
}

void EvolutionController::refreshData() {
  if (rowCount() > 0 && columnCount() > 0) {
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1),
                     { DisplayRole, CategoryNameRole, BudgetRole, SpentRole,
                       LeftoverRole, SavedRole, ReportedRole, AccumulatedRole,
                       CurrentMonthRole, CurrentCategoryRole });
  }
  if (columnCount() > 0) emit headerDataChanged(Qt::Horizontal, 0, columnCount() - 1);
}

void EvolutionController::refreshSelection() {
  if (rowCount() > 0 && columnCount() > 0) {
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1),
                     { CurrentCategoryRole });
  }
  if (rowCount() > 0) emit headerDataChanged(Qt::Vertical, 0, rowCount() - 1);
}
