#include "RuleController.h"

#include <QUndoStack>

#include "Account.h"
#include "BudgetData.h"
#include "CategorizationRule.h"
#include "Operation.h"
#include "RuleListModel.h"
#include "UndoCommands.h"

RuleController::RuleController(BudgetData& budgetData,
                               QUndoStack& undoStack) :
    _budgetData(budgetData),
    _undoStack(undoStack) {
  _ruleModel = new RuleListModel(this);
  _ruleModel->setRuleController(this);
}

RuleController::~RuleController() {
  qDeleteAll(_rules);
}

int RuleController::ruleCount() const {
  return _rules.size();
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
    if (existing->labelPrefix().compare(rule->labelPrefix(), Qt::CaseInsensitive) == 0) {
      qWarning() << "Rule with prefix already exists:" << rule->labelPrefix();
      return;
    }
  }

  rule->setParent(this);
  _rules.append(rule);
  _ruleModel->refresh();
  emit ruleCountChanged();
  emit rulesChanged();
}

void RuleController::addRule(const Category* category, const QString& labelPrefix) {
  if (category == nullptr || labelPrefix.isEmpty()) {
    return;
  }

  auto* rule = new CategorizationRule(category, labelPrefix, this);
  _undoStack.push(new AddRuleCommand(this, rule));
}

void RuleController::removeRule(int index) {
  if (index < 0 || index >= _rules.size()) {
    return;
  }

  _undoStack.push(new RemoveRuleCommand(this, index));
}

void RuleController::editRule(int index, const Category* category, const QString& labelPrefix) {
  if (index < 0 || index >= _rules.size()) {
    return;
  }

  CategorizationRule* rule = _rules[index];
  if (rule->category() == category && rule->labelPrefix() == labelPrefix) {
    return;  // No change
  }

  // Check for duplicate prefix (case-insensitive), excluding the rule being edited
  for (int i = 0; i < _rules.size(); i++) {
    if (i != index && _rules[i]->labelPrefix().compare(labelPrefix, Qt::CaseInsensitive) == 0) {
      qWarning() << "Rule with prefix already exists:" << labelPrefix;
      return;
    }
  }

  _undoStack.push(new EditRuleCommand(this, index,
                                      rule->category(), category,
                                      rule->labelPrefix(), labelPrefix));
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

const Category* RuleController::matchingCategory(Operation* operation) const {
  if (!operation) {
    return nullptr;
  }
  return matchingCategoryForLabel(operation->label());
}

const Category* RuleController::matchingCategoryForLabel(const QString& label) const {
  // Rules are in priority order (first match wins)
  for (const CategorizationRule* rule : _rules) {
    if (rule->matchesLabel(label)) {
      return rule->category();
    }
  }
  return nullptr;
}

int RuleController::applyRulesToOperation(Operation* operation) {
  if (!operation || operation->isCategorized()) {
    return 0;
  }

  if (auto category = matchingCategory(operation)) {
    operation->setAllocations({ CategoryAllocation(category, operation->amount()) });
    return 1;
  }
  return 0;
}

Operation* RuleController::nextUncategorizedOperation(Operation* current) const {
  bool foundCurrent = current == nullptr;
  for (Account* account : _budgetData.accounts()) {
    for (Operation* op : account->operations()) {
      if (op == current) {
        foundCurrent = true;
        continue;
      }
      if (foundCurrent && !op->isCategorized()) {
        return op;
      }
    }
  }
  return nullptr;
}

Operation* RuleController::previousUncategorizedOperation(Operation* current) const {
  if (!current) {
    return nullptr;
  }

  Operation* previousUncategorized = nullptr;
  for (Account* account : _budgetData.accounts()) {
    for (Operation* op : account->operations()) {
      if (op == current) {
        return previousUncategorized;
      }
      if (!op->isCategorized()) {
        previousUncategorized = op;
      }
    }
  }
  return nullptr;
}
