#pragma once

#include <QtQml/qqml.h>
#include <QDate>
#include <QMap>
#include <QObject>
#include <QString>

#include "PropertyMacros.h"

// Action for a leftover decision: save to personal savings or report to leftover pool
enum class LeftoverAction {
  None,    // No decision made yet
  Save,    // Transfer to personal savings
  Report,  // Report to leftover account (carry forward)
};

// Decision made for a specific month's leftover
// Supports partial allocation: some to savings, some to report
struct LeftoverDecision {
  double saveAmount = 0.0;    // Amount transferred to personal savings
  double reportAmount = 0.0;  // Amount carried forward to next month

  bool isEmpty() const { return saveAmount == 0.0 && reportAmount == 0.0; }
  double total() const { return saveAmount + reportAmount; }

  // Legacy compatibility helpers
  LeftoverAction action() const {
    if (saveAmount > 0.0 && reportAmount == 0.0) return LeftoverAction::Save;
    if (reportAmount != 0.0 && saveAmount == 0.0) return LeftoverAction::Report;
    return LeftoverAction::None;
  }
  double amount() const { return saveAmount + reportAmount; }
};

// Key for storing decisions by year-month
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

  static YearMonth fromDate(const QDate& date) {
    return { date.year(), date.month() };
  }
};

class Category : public QObject {
  Q_OBJECT
  QML_ELEMENT
  PROPERTY_RW(QString, name, QString())
  PROPERTY_RW(double, budgetLimit, 0.0)

public:
  explicit Category(QObject* parent = nullptr);
  Category(const QString& name, double budgetLimit, QObject* parent = nullptr);

  // Leftover decision management
  LeftoverDecision leftoverDecision(int year, int month) const;
  void setLeftoverDecision(int year, int month, const LeftoverDecision& decision);
  void clearLeftoverDecision(int year, int month);
  QMap<YearMonth, LeftoverDecision> allLeftoverDecisions() const;

  // Calculate accumulated leftover up to (but not including) a specific month
  // This sums all "Report" decisions from previous months
  double accumulatedLeftoverBefore(int year, int month) const;

signals:
  void leftoverDecisionChanged(int year, int month);

private:
  QMap<YearMonth, LeftoverDecision> _leftoverDecisions;
};
