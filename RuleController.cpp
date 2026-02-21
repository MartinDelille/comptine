#include <QUndoStack>

#include "Account.h"
#include "BudgetData.h"
#include "Operation.h"
#include "Rule.h"
#include "RuleController.h"
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

Rule* RuleController::getRule(int index) const {
  if (index < 0 || index >= _rules.size()) {
    return nullptr;
  }
  return _rules[index];
}

void RuleController::addRule(Rule* rule) {
  if (!rule) {
    return;
  }

  // Check for duplicate (same prefix + same amount filter)
  for (const Rule* existing : _rules) {
    if (existing->labelPrefix().compare(rule->labelPrefix(), Qt::CaseInsensitive) == 0
        && qFuzzyCompare(existing->amountFilter(), rule->amountFilter())) {
      qWarning() << "Rule with same prefix and amount already exists:" << rule->labelPrefix();
      return;
    }
  }

  rule->setParent(this);
  _rules.append(rule);
  _ruleModel->refresh();
  emit ruleCountChanged();
  emit rulesChanged();
}

void RuleController::addRule(const Category* category, const QString& labelPrefix, double amountFilter) {
  if (category == nullptr || labelPrefix.isEmpty()) {
    return;
  }

  auto* rule = new Rule(category, labelPrefix, amountFilter, this);
  _undoStack.push(new AddRuleCommand(this, rule));
}

void RuleController::removeRule(int index) {
  if (index < 0 || index >= _rules.size()) {
    return;
  }

  _undoStack.push(new RemoveRuleCommand(this, index));
}

void RuleController::editRule(int index, const Category* category, const QString& labelPrefix, double amountFilter) {
  if (index < 0 || index >= _rules.size()) {
    return;
  }

  Rule* rule = _rules[index];
  if (rule->category() == category && rule->labelPrefix() == labelPrefix
      && qFuzzyCompare(rule->amountFilter(), amountFilter)) {
    return;  // No change
  }

  // Check for duplicate (same prefix + same amount filter), excluding the rule being edited
  for (int i = 0; i < _rules.size(); i++) {
    if (i != index
        && _rules[i]->labelPrefix().compare(labelPrefix, Qt::CaseInsensitive) == 0
        && qFuzzyCompare(_rules[i]->amountFilter(), amountFilter)) {
      qWarning() << "Rule with same prefix and amount already exists:" << labelPrefix;
      return;
    }
  }

  _undoStack.push(new EditRuleCommand(this, index,
                                      rule->category(), category,
                                      rule->labelPrefix(), labelPrefix,
                                      rule->amountFilter(), amountFilter));
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
  for (const Rule* rule : _rules) {
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

int RuleController::applyRuleToUncategorized(const Category* category, const QString& labelPrefix, double amountFilter) {
  if (!category || labelPrefix.isEmpty()) {
    return 0;
  }

  // Create a temporary rule for matching
  Rule tempRule;
  tempRule.set_category(category);
  tempRule.set_labelPrefix(labelPrefix);
  tempRule.set_amountFilter(amountFilter);

  QUndoCommand* macroCommand = new QUndoCommand();
  int count = 0;

  for (Account* account : _budgetData.accounts()) {
    for (Operation* op : account->operations()) {
      if (!op->isCategorized() && tempRule.matches(op)) {
        QList<Allocation*> newAllocations;
        newAllocations.append(new Allocation(category, op->amount()));
        new SplitOperationCommand(*op, _budgetData.operationModel(),
                                  newAllocations, macroCommand);
        count++;
      }
    }
  }

  if (count > 0) {
    _undoStack.push(macroCommand);
  } else {
    delete macroCommand;
  }

  return count;
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
