#include "Category.h"

Category::Category(QObject* parent) :
    QObject(parent) {}

Category::Category(const QString& name, QObject* parent) :
    QObject(parent), _name(name) {}

// Month history management

MonthRecord Category::monthRecord(const QDate& month) const {
  const QDate key(month.year(), month.month(), 1);
  return _monthHistory.value(key, MonthRecord{});
}

void Category::setMonthRecord(const QDate& month, const MonthRecord& record) {
  const QDate key(month.year(), month.month(), 1);
  if (record.isEmpty()) {
    _monthHistory.remove(key);
  } else {
    _monthHistory[key] = record;
  }
  emit monthHistoryChanged(key);
}

void Category::clearMonthRecord(const QDate& month) {
  const QDate key(month.year(), month.month(), 1);
  if (_monthHistory.remove(key) > 0) {
    emit monthHistoryChanged(key);
  }
}

QMap<QDate, MonthRecord> Category::allMonthHistory() const {
  return _monthHistory;
}

// Legacy leftover decision accessors (convenience wrappers)

LeftoverDecision Category::leftoverDecision(const QDate& month) const {
  return monthRecord(month);
}

void Category::setLeftoverDecision(const QDate& month, const LeftoverDecision& decision) {
  const QDate key(month.year(), month.month(), 1);
  MonthRecord record = _monthHistory.value(key, MonthRecord{});
  record.saveAmount = decision.saveAmount;
  record.reportAmount = decision.reportAmount;
  // Preserve existing budgetLimit if any
  if (record.isEmpty()) {
    _monthHistory.remove(key);
  } else {
    _monthHistory[key] = record;
  }
  emit monthHistoryChanged(key);
}

void Category::clearLeftoverDecision(const QDate& month) {
  const QDate key(month.year(), month.month(), 1);
  auto it = _monthHistory.find(key);
  if (it != _monthHistory.end()) {
    // Only clear leftover data, preserve budget limit if set
    it->saveAmount = 0.0;
    it->reportAmount = 0.0;
    if (it->isEmpty()) {
      _monthHistory.erase(it);
    }
    emit monthHistoryChanged(key);
  }
}

// Budget limit for a specific month. History entries are effective from their
// own month until the next budget-limit entry.
double Category::budgetLimitForMonth(const QDate& date) const {
  const QDate target(date.year(), date.month(), 1);

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
  return monthRecord(date).budgetLimit.has_value();
}

void Category::setBudgetLimitForMonth(const QDate& date, double limit) {
  const QDate key(date.year(), date.month(), 1);
  MonthRecord record = _monthHistory.value(key, MonthRecord{});
  record.budgetLimit = limit;
  _monthHistory[key] = record;
  emit monthHistoryChanged(QDate(date.year(), date.month(), 1));
}

void Category::clearBudgetLimitForMonth(const QDate& month) {
  const QDate key(month.year(), month.month(), 1);
  auto it = _monthHistory.find(key);
  if (it != _monthHistory.end()) {
    it->budgetLimit.reset();
    if (it->isEmpty()) {
      _monthHistory.erase(it);
    }
    emit monthHistoryChanged(key);
  }
}

double Category::accumulatedLeftoverBefore(const QDate& date) const {
  double total = 0.0;
  for (auto it = _monthHistory.constBegin(); it != _monthHistory.constEnd(); ++it) {
    const QDate& month = it.key();
    // Only count decisions before the specified month
    if (month <= QDate(date.year(), date.month(), 1)) {
      // Only reported amounts carry forward
      total += it.value().reportAmount;
    }
  }
  return total;
}
