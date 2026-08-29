#include "RuleEditor.h"

#include "BudgetData.h"
#include "RuleController.h"
#include "UndoCommands.h"
#include "model/Account.h"
#include "model/Operation.h"
#include "model/Rule.h"

RuleEditor::RuleEditor(RuleController& controller, BudgetData& budgetData,
                       QUndoStack& undoStack, QObject* parent) :
    QObject(parent), _controller(controller), _budgetData(budgetData), _undoStack(undoStack) {
}

void RuleEditor::add(const Category* category, const QString& labelMatch, double amountFilter) {
  if (!category || labelMatch.isEmpty()) return;
  for (auto* existing : _controller.rules()) {
    if (existing->labelMatch().compare(labelMatch, Qt::CaseInsensitive) == 0 && qFuzzyCompare(existing->amountFilter(), amountFilter)) return;
  }
  _undoStack.push(new AddRuleCommand(&_controller,
                                     new Rule(category, labelMatch, amountFilter)));
}

void RuleEditor::remove(int index) {
  if (index >= 0 && index < _controller.rules().size())
    _undoStack.push(new RemoveRuleCommand(&_controller, index));
}

void RuleEditor::edit(int index, const Category* category, const QString& labelMatch,
                      double amountFilter) {
  auto* rule = _controller.at(index);
  if (!rule || !category || labelMatch.isEmpty()) return;
  if (rule->category() == category && rule->labelMatch() == labelMatch && qFuzzyCompare(rule->amountFilter(), amountFilter)) return;
  for (auto* existing : _controller.rules()) {
    if (existing != rule && existing->labelMatch().compare(labelMatch, Qt::CaseInsensitive) == 0 && qFuzzyCompare(existing->amountFilter(), amountFilter)) return;
  }
  _undoStack.push(new EditRuleCommand(_controller, rule, category, labelMatch, amountFilter));
}

void RuleEditor::move(int fromIndex, int toIndex) {
  if (fromIndex >= 0 && fromIndex < _controller.rules().size() && toIndex >= 0 && toIndex < _controller.rules().size() && fromIndex != toIndex)
    _undoStack.push(new MoveRuleCommand(_controller, fromIndex, toIndex));
}

int RuleEditor::applyToUncategorized(const Category* category, const QString& labelMatch,
                                     double amountFilter) {
  if (!category || labelMatch.isEmpty()) return 0;

  Rule temporaryRule(category, labelMatch, amountFilter);
  auto* macro = new QUndoCommand();
  int count = 0;
  for (auto* account : _budgetData.accounts()) {
    for (auto* operation : account->operations()) {
      if (!operation->isCategorized() && temporaryRule.matches(operation)) {
        new SplitOperationCommand(*operation,
                                  { new Allocation(category, operation->amount()) },
                                  macro);
        ++count;
      }
    }
  }
  if (count > 0)
    _undoStack.push(macro);
  else
    delete macro;
  return count;
}
