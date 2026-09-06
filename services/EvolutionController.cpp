#include "EvolutionController.h"

#include <QLocale>

#include "BudgetData.h"
#include "CategoryController.h"
#include "model/Operation.h"

EvolutionController::EvolutionController(BudgetData& budgetData,
                                         CategoryController& categories,
                                         QObject* parent) :
    QAbstractTableModel(parent), _budgetData(budgetData), _categories(categories) {
  connect(&_budgetData, &BudgetData::budgetDateChanged, this,
          &EvolutionController::refreshMonthSelection);
  connect(&_categories, &CategoryController::countChanged, this,
          &EvolutionController::refreshCategoryStructure);
  connect(&_categories, &CategoryController::evolutionDataChanged, this,
          &EvolutionController::refreshData);
  connect(&_categories, &CategoryController::currentChanged, this,
          &EvolutionController::refreshSelection);
  connect(this, &EvolutionController::selectedMetricChanged,
          this, &EvolutionController::refreshSummary);
  updateAvailableMonths();
}

QDate EvolutionController::monthDate(int column) const {
  return _availableMonths.value(column);
}

int EvolutionController::monthCount() const {
  return _availableMonths.size();
}

QStringList EvolutionController::availableMonthLabels() const {
  QStringList labels;
  const QLocale locale;
  for (const QDate& month : _availableMonths) {
    labels.append(locale.toString(month, "MMMM yyyy"));
  }
  return labels;
}

int EvolutionController::summaryStartIndex() const {
  return _availableMonths.indexOf(summaryStartMonth());
}

int EvolutionController::summaryEndIndex() const {
  return _availableMonths.indexOf(summaryEndMonth());
}

void EvolutionController::setSummaryStartMonthIndex(int index) {
  set_summaryStartMonth(monthDate(index));
}

void EvolutionController::setSummaryEndMonthIndex(int index) {
  set_summaryEndMonth(monthDate(index));
}

int EvolutionController::currentMonthIndex() const {
  const QDate selected(_budgetData.budgetDate().year(), _budgetData.budgetDate().month(), 1);
  return _availableMonths.indexOf(selected);
}

QDate EvolutionController::summaryStartMonth() const {
  return _summaryStartMonth;
}

QDate EvolutionController::summaryEndMonth() const {
  return _summaryEndMonth;
}

void EvolutionController::set_summaryStartMonth(QDate value) {
  const QDate month = clampMonth(value);
  if (!month.isValid() || month == _summaryStartMonth) {
    return;
  }

  _summaryRangeCustomized = true;
  _summaryStartMonth = month;
  emit summaryStartMonthChanged();
  if (_summaryEndMonth.isValid() && _summaryEndMonth < month) {
    _summaryEndMonth = month;
    emit summaryEndMonthChanged();
  }
  refreshSummary();
}

void EvolutionController::set_summaryEndMonth(QDate value) {
  const QDate month = clampMonth(value);
  if (!month.isValid() || month == _summaryEndMonth) {
    return;
  }

  _summaryRangeCustomized = true;
  _summaryEndMonth = month;
  emit summaryEndMonthChanged();
  if (_summaryStartMonth.isValid() && _summaryStartMonth > month) {
    _summaryStartMonth = month;
    emit summaryStartMonthChanged();
  }
  refreshSummary();
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

bool EvolutionController::updateAvailableMonths() {
  const QList<QDate> months = calculateAvailableMonths();
  if (months == _availableMonths) {
    return false;
  }

  const QDate previousStart = summaryStartMonth();
  const QDate previousEnd = summaryEndMonth();
  _availableMonths = months;
  const QDate newStart = _summaryRangeCustomized && previousStart.isValid()
                             ? clampMonth(previousStart)
                             : _availableMonths.first();
  const QDate newEnd = _summaryRangeCustomized && previousEnd.isValid()
                           ? clampMonth(previousEnd)
                           : _availableMonths.constLast();
  if (newStart != summaryStartMonth()) {
    _summaryStartMonth = newStart;
    emit summaryStartMonthChanged();
  }
  if (newEnd != summaryEndMonth()) {
    _summaryEndMonth = newEnd;
    emit summaryEndMonthChanged();
  }
  emit availableMonthsChanged();
  emit currentMonthIndexChanged();
  emit firstMonthChanged();
  emit lastMonthChanged();
  resetModel();
  return true;
}

int EvolutionController::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : _categories.categories().size();
}

int EvolutionController::columnCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : _availableMonths.size();
}

double EvolutionController::metricValue(const Category* category,
                                        const QDate& month,
                                        int metric) const {
  switch (metric) {
    case 0:
      return category->budgetLimitForMonth(month);
    case 1:
      return _categories.spentInCategory(category, month);
    case 2:
      return _categories.leftoverForCategory(category, month);
    case 3:
      return category->monthRecord(month.year(), month.month()).saveAmount;
    case 4:
      return category->monthRecord(month.year(), month.month()).reportAmount;
    case 5:
      return category->accumulatedLeftoverBefore(month);
    default:
      return category->budgetLimitForMonth(month);
  }
}

QDate EvolutionController::clampMonth(const QDate& month) const {
  if (_availableMonths.isEmpty()) {
    return {};
  }
  if (!month.isValid() || month < _availableMonths.first()) {
    return _availableMonths.first();
  }
  if (month > _availableMonths.constLast()) {
    return _availableMonths.constLast();
  }
  return QDate(month.year(), month.month(), 1);
}

int EvolutionController::summaryMonthCount() const {
  const QDate start = summaryStartMonth();
  const QDate end = summaryEndMonth();
  if (!start.isValid() || !end.isValid() || start > end) {
    return 0;
  }
  return (end.year() - start.year()) * 12 + end.month() - start.month() + 1;
}

double EvolutionController::categorySum(const Category* category) const {
  double total = 0.0;
  const QDate start = summaryStartMonth();
  const QDate end = summaryEndMonth();
  if (!start.isValid() || !end.isValid() || start > end) {
    return 0.0;
  }
  for (QDate month = start; month <= end; month = month.addMonths(1)) {
    total += metricValue(category, month, selectedMetric());
  }
  return total;
}

QVariant EvolutionController::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.column() < 0 || index.column() >= columnCount()) {
    return {};
  }

  auto* category = _categories.at(index.row());
  if (!category) {
    return {};
  }
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
    case CategoryAverageRole:
      return summaryMonthCount() == 0 ? 0.0 : categorySum(category) / summaryMonthCount();
    case CategorySumRole:
      return categorySum(category);
    case CurrentMonthRole: {
      const QDate selected(_budgetData.budgetDate().year(), _budgetData.budgetDate().month(), 1);
      return monthDate(index.column()) == selected;
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
    if (role == CategoryAverageRole) {
      return summaryMonthCount() == 0 ? 0.0 : categorySum(_categories.categories().at(section)) / summaryMonthCount();
    }
    if (role == CategorySumRole) {
      return categorySum(_categories.categories().at(section));
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
    { CategoryAverageRole, "categoryAverage" },
    { CategorySumRole, "categorySum" },
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
  if (updateAvailableMonths()) {
    return;
  }

  if (rowCount() > 0 && columnCount() > 0) {
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1),
                     { DisplayRole, CategoryNameRole, BudgetRole, SpentRole,
                       LeftoverRole, SavedRole, ReportedRole, AccumulatedRole,
                       CategoryAverageRole, CategorySumRole,
                       CurrentMonthRole, CurrentCategoryRole });
  }
  if (columnCount() > 0) emit headerDataChanged(Qt::Horizontal, 0, columnCount() - 1);
  if (rowCount() > 0) emit headerDataChanged(Qt::Vertical, 0, rowCount() - 1);
}

void EvolutionController::refreshCategoryStructure() {
  if (!updateAvailableMonths()) {
    resetModel();
  }
}

void EvolutionController::refreshMonthSelection() {
  if (updateAvailableMonths()) {
    return;
  }

  emit currentMonthIndexChanged();
  if (rowCount() > 0 && columnCount() > 0) {
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1),
                     { CurrentMonthRole });
  }
  if (columnCount() > 0) {
    emit headerDataChanged(Qt::Horizontal, 0, columnCount() - 1);
  }
}

void EvolutionController::refreshSelection() {
  if (rowCount() > 0 && columnCount() > 0) {
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1),
                     { CurrentCategoryRole });
  }
  if (rowCount() > 0) emit headerDataChanged(Qt::Vertical, 0, rowCount() - 1);
}

void EvolutionController::refreshSummary() {
  if (rowCount() > 0) {
    emit headerDataChanged(Qt::Vertical, 0, rowCount() - 1);
  }
}
