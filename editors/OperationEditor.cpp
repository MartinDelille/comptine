#include "OperationEditor.h"

#include "BudgetData.h"
#include "UndoCommands.h"
#include "model/Account.h"
#include "model/Operation.h"

OperationEditor::OperationEditor(BudgetData& budgetData, QUndoStack& undoStack, QObject* parent) :
    QObject(parent), _budgetData(budgetData), _undoStack(undoStack) {
}

void OperationEditor::add(const QDate& date, double amount, const QString& label,
                          const QString& details, const QList<Allocation*>& allocations) {
  auto* account = _budgetData.currentAccount();
  if (!account) {
    qDeleteAll(allocations);
    return;
  }
  auto* operation = new Operation(account, date, amount, label, details, allocations);
  _undoStack.push(new AddOperationCommand(operation, *account));
}

void OperationEditor::setBudgetDate(Operation* operation, const QDate& date) {
  if (operation && operation->budgetDate() != date)
    _undoStack.push(new SetOperationBudgetDateCommand(*operation, date));
}

void OperationEditor::setAmount(Operation* operation, double amount) {
  if (operation && !qFuzzyCompare(operation->amount(), amount))
    _undoStack.push(new SetOperationAmountCommand(*operation, amount));
}

void OperationEditor::setDate(Operation* operation, const QDate& date) {
  if (operation && operation->date() != date)
    _undoStack.push(new SetOperationDateCommand(*operation, date));
}

void OperationEditor::setLabel(Operation* operation, const QString& label) {
  if (operation && operation->label() != label)
    _undoStack.push(new SetOperationLabelCommand(*operation, label));
}

void OperationEditor::setDetails(Operation* operation, const QString& details) {
  if (operation && operation->details() != details)
    _undoStack.push(new SetOperationDetailsCommand(*operation, details));
}

void OperationEditor::setAllocations(Operation* operation, const QList<Allocation*>& allocations) {
  if (!operation) return;
  if (operation->sameAllocations(allocations)) {
    qDeleteAll(allocations);
    return;
  }
  _undoStack.push(new SplitOperationCommand(*operation, allocations));
}

Operation* OperationEditor::createCounterpart(Operation* operation, Account* account,
                                              const QString& categoryName) {
  if (!operation || !account) return nullptr;
  QList<Allocation*> allocations;
  double amount = 0.0;
  for (auto* allocation : operation->allocations()) {
    if (categoryName.isEmpty() || (allocation->category() && allocation->category()->name() == categoryName)) {
      amount -= allocation->amount();
      allocations.append(new Allocation(allocation->category(), -allocation->amount()));
    }
  }
  auto* counterpart = new Operation(account, operation->date(), amount,
                                    operation->label(), operation->details(), allocations);
  _undoStack.push(new AddOperationCommand(counterpart, *account));
  return counterpart;
}

void OperationEditor::deleteSelected() {
  auto* account = _budgetData.currentAccount();
  if (!account) return;
  auto* macro = new QUndoCommand();
  for (auto* operation : account->selectedOperations())
    new DeleteOperationCommand(operation, *account, macro);
  _undoStack.push(macro);
}
