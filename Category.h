#pragma once

#include <QtQml/qqml.h>
#include <QDate>
#include <QMap>
#include <QObject>
#include <QString>

#include <optional>

#include "PropertyMacros.h"

// Key for storing per-month data by year-month
struct YearMonth {
  int year = 0;
  int month = 0;

  bool operator<(const YearMonth& other) const {
    if (year != other.year) return year < other.year;
    return month < other.month;
  }

  bool operator==(const YearMonth& other) const {
    return year == other.year && month == other.month;
  }

  bool operator<=(const YearMonth& other) const {
    return *this < other || *this == other;
  }

  static YearMonth fromDate(const QDate& date) {
    return { date.year(), date.month() };
  }
};

// Per-month record for a category: leftover decisions and optional budget limit override
struct MonthRecord {
  double saveAmount = 0.0;            // Amount transferred to personal savings
  double reportAmount = 0.0;          // Amount carried forward to next month
  std::optional<double> budgetLimit;  // Budget limit effective during this month (if changed)

  bool isEmpty() const {
    return saveAmount == 0.0 && reportAmount == 0.0 && !budgetLimit.has_value();
  }

  bool hasLeftoverData() const {
    return saveAmount != 0.0 || reportAmount != 0.0;
  }

  double leftoverTotal() const { return saveAmount + reportAmount; }
};

// Legacy alias for backward compatibility in code that only deals with leftover data
using LeftoverDecision = MonthRecord;

class Category : public QObject {
  Q_OBJECT
  QML_ELEMENT
  PROPERTY_RW(QString, name, QString())
  PROPERTY_RW(double, budgetLimit, 0.0)

public:
  explicit Category(QObject* parent = nullptr);
  Category(const QString& name, double budgetLimit = 0.0, QObject* parent = nullptr);

  // Month history management (leftover decisions + budget limit overrides)
  MonthRecord monthRecord(int year, int month) const;
  void setMonthRecord(int year, int month, const MonthRecord& record);
  void clearMonthRecord(int year, int month);
  QMap<YearMonth, MonthRecord> allMonthHistory() const;

  // Legacy leftover decision accessors (convenience wrappers)
  LeftoverDecision leftoverDecision(int year, int month) const;
  void setLeftoverDecision(int year, int month, const LeftoverDecision& decision);
  void clearLeftoverDecision(int year, int month);

  // Budget limit for a specific month
  // Looks up month_history for the effective budget limit at that date.
  // If no historical entry is found, returns the current budgetLimit().
  Q_INVOKABLE double budgetLimitForMonth(const QDate& date) const;

  // Set a historical budget limit for a specific month
  void setBudgetLimitForMonth(int year, int month, double limit);

  // Clear a historical budget limit for a specific month (revert to current)
  void clearBudgetLimitForMonth(int year, int month);

  // Calculate accumulated leftover up to (but not including) a specific month
  // This sums all "Report" decisions from previous months
  double accumulatedLeftoverBefore(const QDate& date) const;

signals:
  void monthHistoryChanged(int year, int month);

private:
  QMap<YearMonth, MonthRecord> _monthHistory;
};
