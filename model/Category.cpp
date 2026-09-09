#include "Category.h"

Category::Category(QObject* parent) :
    QObject(parent) {}

Category::Category(const QString& name, QObject* parent) :
    QObject(parent), _name(name) {}

// Month history management

MonthRecord Category::monthRecord(int year, int month) const {
  YearMonth key{ year, month };
  return _monthHistory.value(key, MonthRecord{});
}

void Category::setMonthRecord(int year, int month, const MonthRecord& record) {
  YearMonth key{ year, month };
  if (record.isEmpty()) {
    _monthHistory.remove(key);
  } else {
    _monthHistory[key] = record;
  }
  emit monthHistoryChanged(year, month);
}

void Category::clearMonthRecord(int year, int month) {
  YearMonth key{ year, month };
  if (_monthHistory.remove(key) > 0) {
    emit monthHistoryChanged(year, month);
  }
}

QMap<YearMonth, MonthRecord> Category::allMonthHistory() const {
  return _monthHistory;
}

// Legacy leftover decision accessors (convenience wrappers)

LeftoverDecision Category::leftoverDecision(int year, int month) const {
  return monthRecord(year, month);
}

void Category::setLeftoverDecision(int year, int month, const LeftoverDecision& decision) {
  YearMonth key{ year, month };
  MonthRecord record = _monthHistory.value(key, MonthRecord{});
  record.saveAmount = decision.saveAmount;
  record.reportAmount = decision.reportAmount;
  // Preserve existing budgetLimit if any
  if (record.isEmpty()) {
    _monthHistory.remove(key);
  } else {
    _monthHistory[key] = record;
  }
  emit monthHistoryChanged(year, month);
}

void Category::clearLeftoverDecision(int year, int month) {
  YearMonth key{ year, month };
  auto it = _monthHistory.find(key);
  if (it != _monthHistory.end()) {
    // Only clear leftover data, preserve budget limit if set
    it->saveAmount = 0.0;
    it->reportAmount = 0.0;
    if (it->isEmpty()) {
      _monthHistory.erase(it);
    }
    emit monthHistoryChanged(year, month);
  }
}

// Budget limit for a specific month. History entries are effective from their
// own month until the next budget-limit entry.
double Category::budgetLimitForMonth(const QDate& date) const {
  YearMonth target = YearMonth::fromDate(date);

  for (auto it = _monthHistory.upperBound(target); it != _monthHistory.begin();) {
    --it;
    if (it.value().budgetLimit.has_value()) {
      return it.value().budgetLimit.value();
    }
  }

  // No historical entry at or before this date: no budget is defined yet.
  return 0.0;
}

bool Category::hasBudgetLimitOverrideForMonth(const QDate& date) const {
  return monthRecord(date.year(), date.month()).budgetLimit.has_value();
}

void Category::setBudgetLimitForMonth(int year, int month, double limit) {
  YearMonth key{ year, month };
  MonthRecord record = _monthHistory.value(key, MonthRecord{});
  record.budgetLimit = limit;
  _monthHistory[key] = record;
  emit monthHistoryChanged(year, month);
}

void Category::clearBudgetLimitForMonth(int year, int month) {
  YearMonth key{ year, month };
  auto it = _monthHistory.find(key);
  if (it != _monthHistory.end()) {
    it->budgetLimit.reset();
    if (it->isEmpty()) {
      _monthHistory.erase(it);
    }
    emit monthHistoryChanged(year, month);
  }
}

double Category::accumulatedLeftoverBefore(const QDate& date) const {
  double total = 0.0;
  for (auto it = _monthHistory.constBegin(); it != _monthHistory.constEnd(); ++it) {
    const YearMonth& ym = it.key();
    // Only count decisions before the specified month
    if (ym.year < date.year() || (ym.year == date.year() && ym.month <= date.month())) {
      // Only reported amounts carry forward
      total += it.value().reportAmount;
    }
  }
  return total;
}
