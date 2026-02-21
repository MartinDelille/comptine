#include "CategorizationRule.h"

#include "Operation.h"

CategorizationRule::CategorizationRule(QObject* parent) :
    QObject(parent) {
}

CategorizationRule::CategorizationRule(const Category* category,
                                       const QString& labelPrefix,
                                       QObject* parent) :
    QObject(parent) {
  _category = category;
  _labelPrefix = labelPrefix;
}

CategorizationRule::CategorizationRule(const Category* category,
                                       const QString& labelPrefix,
                                       double amountFilter,
                                       QObject* parent) :
    QObject(parent) {
  _category = category;
  _labelPrefix = labelPrefix;
  _amountFilter = amountFilter;
}

bool CategorizationRule::matches(Operation* operation) const {
  if (!operation) {
    return false;
  }
  if (_labelPrefix.isEmpty()) {
    return false;
  }
  // Case-insensitive prefix match
  if (!operation->label().startsWith(_labelPrefix, Qt::CaseInsensitive)) {
    return false;
  }
  // If amount filter is set, check it matches
  if (_amountFilter != 0) {
    if (!qFuzzyCompare(_amountFilter, operation->amount())) {
      return false;
    }
  }
  return true;
}
