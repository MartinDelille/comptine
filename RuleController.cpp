#include "RuleController.h"

#include <QUndoStack>

#include "Account.h"
#include "BudgetData.h"
#include "CategorizationRule.h"
#include "Operation.h"
#include "RuleListModel.h"
#include "UndoCommands.h"

RuleController::RuleController(QUndoStack& undoStack, QObject* parent) : QObject(parent), _undoStack(undoStack) {
  _ruleModel = new RuleListModel(this);
  _ruleModel->setRuleController(this);
}

RuleController::~RuleController() {
  qDeleteAll(_rules);
}

void RuleController::setBudgetData(BudgetData* budgetData) {
  _budgetData = budgetData;
  updateUncategorizedCount();

  // Update uncategorized count when data changes
  if (_budgetData) {
    connect(_budgetData, &BudgetData::accountCountChanged, this, &RuleController::updateUncategorizedCount);
    connect(_budgetData, &BudgetData::operationDataChanged, this, &RuleController::updateUncategorizedCount);
  }
}

int RuleController::ruleCount() const {
  return _rules.size();
}

int RuleController::uncategorizedCount() const {
  if (!_budgetData) {
    return 0;
  }

  int count = 0;
  for (Account* account : _budgetData->accounts()) {
    for (Operation* op : account->operations()) {
      if (op->category().isEmpty() && !op->isSplit()) {
        count++;
      }
    }
  }
  return count;
}

CategorizationRule* RuleController::getRule(int index) const {
  if (index < 0 || index >= _rules.size()) {
    return nullptr;
  }
  return _rules[index];
}

void RuleController::addRule(CategorizationRule* rule) {
  if (!rule) {
    return;
  }

  // Check for duplicate prefix (case-insensitive)
  for (const CategorizationRule* existing : _rules) {
    if (existing->descriptionPrefix().compare(rule->descriptionPrefix(), Qt::CaseInsensitive) == 0) {
      qWarning() << "Rule with prefix already exists:" << rule->descriptionPrefix();
      return;
    }
  }

  rule->setParent(this);
  _rules.append(rule);
  _ruleModel->refresh();
  emit ruleCountChanged();
  emit rulesChanged();
}

void RuleController::addRule(const QString& category, const QString& descriptionPrefix) {
  if (category.isEmpty() || descriptionPrefix.isEmpty()) {
    return;
  }

  auto* rule = new CategorizationRule(category, descriptionPrefix, this);
  _undoStack.push(new AddRuleCommand(this, rule));
}

void RuleController::removeRule(int index) {
  if (index < 0 || index >= _rules.size()) {
    return;
  }

  _undoStack.push(new RemoveRuleCommand(this, index));
}

void RuleController::editRule(int index, const QString& category, const QString& descriptionPrefix) {
  if (index < 0 || index >= _rules.size()) {
    return;
  }

  CategorizationRule* rule = _rules[index];
  if (rule->category() == category && rule->descriptionPrefix() == descriptionPrefix) {
    return;  // No change
  }

  // Check for duplicate prefix (case-insensitive), excluding the rule being edited
  for (int i = 0; i < _rules.size(); i++) {
    if (i != index && _rules[i]->descriptionPrefix().compare(descriptionPrefix, Qt::CaseInsensitive) == 0) {
      qWarning() << "Rule with prefix already exists:" << descriptionPrefix;
      return;
    }
  }

  _undoStack.push(new EditRuleCommand(this, index,
                                      rule->category(), category,
                                      rule->descriptionPrefix(), descriptionPrefix));
}

void RuleController::moveRule(int fromIndex, int toIndex) {
  if (fromIndex < 0 || fromIndex >= _rules.size()) {
    return;
  }
  if (toIndex < 0 || toIndex >= _rules.size()) {
    return;
  }
  if (fromIndex == toIndex) {
    return;
  }

  _undoStack.push(new MoveRuleCommand(this, fromIndex, toIndex));
}

void RuleController::moveRuleDirect(int fromIndex, int toIndex) {
  if (fromIndex < 0 || fromIndex >= _rules.size()) {
    return;
  }
  if (toIndex < 0 || toIndex >= _rules.size()) {
    return;
  }
  if (fromIndex == toIndex) {
    return;
  }

  CategorizationRule* rule = _rules.takeAt(fromIndex);
  _rules.insert(toIndex, rule);

  _ruleModel->refresh();
  emit rulesChanged();
}

void RuleController::clearRules() {
  qDeleteAll(_rules);
  _rules.clear();
  _ruleModel->refresh();
  emit ruleCountChanged();
  emit rulesChanged();
}

CategorizationRule* RuleController::takeRule(int index) {
  if (index < 0 || index >= _rules.size()) {
    return nullptr;
  }

  CategorizationRule* rule = _rules.takeAt(index);
  _ruleModel->refresh();
  emit ruleCountChanged();
  emit rulesChanged();
  return rule;
}

QString RuleController::matchingCategory(Operation* operation) const {
  if (!operation) {
    return QString();
  }
  return matchingCategoryForDescription(operation->description());
}

QString RuleController::matchingCategoryForDescription(const QString& description) const {
  // Rules are in priority order (first match wins)
  for (const CategorizationRule* rule : _rules) {
    if (rule->matchesDescription(description)) {
      return rule->category();
    }
  }
  return QString();
}

int RuleController::applyRulesToOperation(Operation* operation) {
  if (!operation || !operation->category().isEmpty() || operation->isSplit()) {
    return 0;
  }

  QString category = matchingCategory(operation);
  if (!category.isEmpty()) {
    operation->set_category(category);
    return 1;
  }
  return 0;
}

QList<Operation*> RuleController::uncategorizedOperations() const {
  QList<Operation*> result;
  if (!_budgetData) {
    return result;
  }

  for (Account* account : _budgetData->accounts()) {
    for (Operation* op : account->operations()) {
      if (op->category().isEmpty() && !op->isSplit()) {
        result.append(op);
      }
    }
  }
  return result;
}

Operation* RuleController::nextUncategorizedOperation() const {
  QList<Operation*> ops = uncategorizedOperations();
  return ops.isEmpty() ? nullptr : ops.first();
}

void RuleController::updateUncategorizedCount() {
  emit uncategorizedCountChanged();
}
