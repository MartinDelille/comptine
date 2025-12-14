#include "Category.h"

Category::Category(QObject* parent) :
    QObject(parent) {}

Category::Category(const QString& name, double budgetLimit, QObject* parent) :
    QObject(parent), _name(name), _budgetLimit(budgetLimit) {}

LeftoverDecision Category::leftoverDecision(int year, int month) const {
  YearMonth key{ year, month };
  return _leftoverDecisions.value(key, LeftoverDecision{});
}

void Category::setLeftoverDecision(int year, int month, const LeftoverDecision& decision) {
  YearMonth key{ year, month };
  _leftoverDecisions[key] = decision;
  emit leftoverDecisionChanged(year, month);
}

void Category::clearLeftoverDecision(int year, int month) {
  YearMonth key{ year, month };
  if (_leftoverDecisions.remove(key) > 0) {
    emit leftoverDecisionChanged(year, month);
  }
}

QMap<YearMonth, LeftoverDecision> Category::allLeftoverDecisions() const {
  return _leftoverDecisions;
}

double Category::accumulatedLeftoverBefore(int year, int month) const {
  double total = 0.0;
  for (auto it = _leftoverDecisions.constBegin(); it != _leftoverDecisions.constEnd(); ++it) {
    const YearMonth& ym = it.key();
    // Only count decisions before the specified month
    if (ym.year < year || (ym.year == year && ym.month < month)) {
      // Only reported amounts carry forward
      total += it.value().reportAmount;
    }
  }
  return total;
}
