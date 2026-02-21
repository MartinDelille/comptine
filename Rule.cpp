#include "Rule.h"

#include "Operation.h"

Rule::Rule(QObject* parent) :
    QObject(parent) {
}

Rule::Rule(const Category* category,
           const QString& labelPrefix,
           QObject* parent) :
    QObject(parent) {
  _category = category;
  _labelPrefix = labelPrefix;
}

Rule::Rule(const Category* category,
           const QString& labelPrefix,
           double amountFilter,
           QObject* parent) :
    QObject(parent) {
  _category = category;
  _labelPrefix = labelPrefix;
  _amountFilter = amountFilter;
}

bool Rule::matches(Operation* operation) const {
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
