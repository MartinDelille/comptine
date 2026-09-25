#pragma once

#include <QtQml/qqml.h>
#include <QDate>
#include <QMap>
#include <QObject>
#include <QString>

#include <optional>

#include "utils/PropertyMacros.h"

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
  QML_UNCREATABLE("Categories are created by the backend")
  PROPERTY_RW(QString, name, QString())

public:
  explicit Category(QObject* parent = nullptr);
  explicit Category(const QString& name, QObject* parent = nullptr);

  // Month history management (leftover decisions + budget limit overrides)
  MonthRecord monthRecord(const QDate& month) const;
  void setMonthRecord(const QDate& month, const MonthRecord& record);
  void clearMonthRecord(const QDate& month);
  QMap<QDate, MonthRecord> allMonthHistory() const;

  // Legacy leftover decision accessors (convenience wrappers)
  LeftoverDecision leftoverDecision(const QDate& month) const;
  void setLeftoverDecision(const QDate& month, const LeftoverDecision& decision);
  void clearLeftoverDecision(const QDate& month);

  // Budget limit for a specific month
  // Looks up month_history for the effective budget limit at that date.
  // If no historical entry is found, returns zero.
  Q_INVOKABLE double budgetLimitForMonth(const QDate& date) const;

  Q_INVOKABLE bool hasBudgetLimitOverrideForMonth(const QDate& date) const;

  // Set a historical budget limit for a specific month
  void setBudgetLimitForMonth(const QDate& date, double limit);

  // Clear a historical budget limit for a specific month (inherit from before)
  void clearBudgetLimitForMonth(const QDate& month);

  // Calculate accumulated leftover up to (but not including) a specific month
  // This sums all "Report" decisions from previous months
  double accumulatedLeftoverBefore(const QDate& date) const;

signals:
  void monthHistoryChanged(const QDate& month);

private:
  QMap<QDate, MonthRecord> _monthHistory;
};
