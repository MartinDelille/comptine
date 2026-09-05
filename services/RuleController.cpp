#include <QUndoStack>

#include "BudgetData.h"
#include "RuleController.h"
#include "RuleListModel.h"
#include "UndoCommands.h"
#include "model/Operation.h"
#include "model/Rule.h"

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

Rule* RuleController::at(int index) const {
  if (index < 0 || index >= _rules.size()) {
    return nullptr;
  }
  return _rules[index];
}

void RuleController::addRule(Rule* rule) {
  if (!rule) {
    return;
  }

  // Check for duplicate (same match + same amount filter)
  for (auto existing : _rules) {
    if (existing->labelMatch().compare(rule->labelMatch(), Qt::CaseInsensitive) == 0
        && qFuzzyCompare(existing->amountFilter(), rule->amountFilter())) {
      qWarning() << "Rule with same match and amount already exists:" << rule->labelMatch();
      return;
    }
  }

  rule->setParent(this);
  _rules.append(rule);
  _ruleModel->refresh();
  emit ruleCountChanged();
  emit rulesChanged();
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

  Rule* rule = _rules.takeAt(fromIndex);
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

Rule* RuleController::takeRule(int index) {
  if (index < 0 || index >= _rules.size()) {
    return nullptr;
  }

  Rule* rule = _rules.takeAt(index);
  _ruleModel->refresh();
  emit ruleCountChanged();
  emit rulesChanged();
  return rule;
}

const Category* RuleController::matchingCategory(Operation* operation) const {
  if (!operation) {
    return nullptr;
  }
  // Use full matching (label + optional amount) so amount-filtered rules work
  for (auto rule : _rules) {
    if (rule->matches(operation)) {
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
    operation->setAllocations({ new Allocation(category, operation->amount()) });
    return 1;
  }
  return 0;
}

Operation* RuleController::nextUncategorizedOperation(Operation* current) const {
  bool foundCurrent = current == nullptr;
  for (auto account : _budgetData.accounts()) {
    for (auto op : account->operations()) {
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
  for (auto account : _budgetData.accounts()) {
    for (auto op : account->operations()) {
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
