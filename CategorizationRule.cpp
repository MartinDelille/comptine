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

bool CategorizationRule::matches(Operation* operation) const {
  if (!operation) {
    return false;
  }
  return matchesLabel(operation->label());
}

bool CategorizationRule::matchesLabel(const QString& label) const {
  if (_labelPrefix.isEmpty()) {
    return false;
  }
  // Case-insensitive prefix match
  return label.startsWith(_labelPrefix, Qt::CaseInsensitive);
}
