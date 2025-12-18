#include "CategorizationRule.h"

#include "Operation.h"

CategorizationRule::CategorizationRule(QObject* parent) :
    QObject(parent) {
}

CategorizationRule::CategorizationRule(const Category* category,
                                       const QString& descriptionPrefix,
                                       QObject* parent) :
    QObject(parent) {
  _category = category;
  _descriptionPrefix = descriptionPrefix;
}

bool CategorizationRule::matches(Operation* operation) const {
  if (!operation) {
    return false;
  }
  return matchesDescription(operation->description());
}

bool CategorizationRule::matchesDescription(const QString& description) const {
  if (_descriptionPrefix.isEmpty()) {
    return false;
  }
  // Case-insensitive prefix match
  return description.startsWith(_descriptionPrefix, Qt::CaseInsensitive);
}
