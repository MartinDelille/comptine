#include "OperationEditor.h"

#include <QHash>
#include <QUndoStack>
#include <QVariant>

#include "BudgetData.h"
#include "UndoCommands.h"
#include "model/Account.h"
#include "model/Operation.h"

namespace {

QList<Allocation*> typedAllocations(const QVariantList& values) {
  QList<Allocation*> allocations;
  for (const auto& value : values) {
    QObject* object = value.value<QObject*>();
    if (auto* allocation = qobject_cast<Allocation*>(object)) {
      allocations.append(allocation);
    }
  }
  return allocations;
}

}  // namespace

OperationEditor::OperationEditor(BudgetData& budgetData, QUndoStack& undoStack, QObject* parent) :
    QObject(parent), _budgetData(budgetData), _undoStack(undoStack) {
  connect(&_undoStack, &QUndoStack::canUndoChanged, this, &OperationEditor::undoStateChanged);
  connect(&_undoStack, &QUndoStack::canRedoChanged, this, &OperationEditor::undoStateChanged);
}

bool OperationEditor::canUndo() const { return !_editingOperation && _undoStack.canUndo(); }
bool OperationEditor::canRedo() const { return !_editingOperation && _undoStack.canRedo(); }

bool OperationEditor::beginEditing(Operation* operation) {
  if (!operation || _editingOperation) {
    emit transactionRejected();
    return false;
  }
  _editingOperation = operation;
  _editingStartCount = _undoStack.count();
  _undoStack.beginMacro(QObject::tr("Edit operation"));
  connect(operation, &QObject::destroyed, this, [this]() {
    if (_editingOperation) endEditing(false);
  });
  emit editingChanged();
  emit editingStarted(operation);
  return true;
}

Operation* OperationEditor::beginNew(const QDate& date, double amount,
                                     const QString& label, const QString& details) {
  if (_editingOperation) {
    emit transactionRejected();
    return nullptr;
  }
  auto* account = _budgetData.currentAccount();
  if (!account) return nullptr;
  _editingStartCount = _undoStack.count();
  _undoStack.beginMacro(QObject::tr("Add operation"));
  auto* operation = new Operation(account, date, amount, label, details, {});
  _undoStack.push(new AddOperationCommand(operation, *account));
  _editingOperation = operation;
  connect(operation, &QObject::destroyed, this, [this]() {
    if (_editingOperation) endEditing(false);
  });
  emit editingChanged();
  emit editingStarted(operation);
  return operation;
}

void OperationEditor::endEditing(bool commit) {
  if (!_editingOperation) {
    emit transactionRejected();
    return;
  }
  const bool hadCommands = _undoStack.count() > _editingStartCount;
  _undoStack.endMacro();
  if (!commit && hadCommands) _undoStack.undo();
  _editingOperation.clear();
  _editingStartCount = -1;
  emit editingChanged();
  if (commit)
    emit editingFinished();
  else
    emit editingCancelled();
  emit undoStateChanged();
}

void OperationEditor::undo() {
  if (_editingOperation) {
    emit undoRejected();
    return;
  }
  _undoStack.undo();
}

void OperationEditor::redo() {
  if (_editingOperation) {
    emit redoRejected();
    return;
  }
  _undoStack.redo();
}

void OperationEditor::add(const QDate& date, double amount, const QString& label,
                          const QString& details, const QVariantList& values) {
  const auto allocations = typedAllocations(values);
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

void OperationEditor::setAllocationList(Operation* operation, const QList<Allocation*>& allocations) {
  if (!operation) {
    qDeleteAll(allocations);
    return;
  }
  if (operation->sameAllocations(allocations)) {
    qDeleteAll(allocations);
    return;
  }
  _undoStack.push(new SplitOperationCommand(*operation, allocations));
}

void OperationEditor::addAllocation(Operation* operation, const Category* category, double amount) {
  if (!operation) return;
  QList<Allocation*> allocations;
  for (auto* allocation : operation->allocations())
    allocations.append(new Allocation(allocation->category(), allocation->amount()));
  allocations.append(new Allocation(category, amount));
  setAllocationList(operation, allocations);
}

void OperationEditor::removeAllocation(Operation* operation, int index) {
  if (!operation || index < 0 || index >= operation->allocations().size()) return;
  QList<Allocation*> allocations;
  for (int i = 0; i < operation->allocations().size(); ++i) {
    if (i != index) {
      auto* allocation = operation->allocations().at(i);
      allocations.append(new Allocation(allocation->category(), allocation->amount()));
    }
  }
  setAllocationList(operation, allocations);
}

void OperationEditor::setAllocationCategory(Operation* operation, int index, const Category* category) {
  if (!operation || index < 0 || index >= operation->allocations().size()) return;
  QList<Allocation*> allocations;
  for (int i = 0; i < operation->allocations().size(); ++i) {
    auto* allocation = operation->allocations().at(i);
    allocations.append(new Allocation(i == index ? category : allocation->category(), allocation->amount()));
  }
  setAllocationList(operation, allocations);
}

void OperationEditor::setAllocationAmount(Operation* operation, int index, double amount) {
  if (!operation || index < 0 || index >= operation->allocations().size()) return;
  QList<Allocation*> allocations;
  for (int i = 0; i < operation->allocations().size(); ++i) {
    auto* allocation = operation->allocations().at(i);
    allocations.append(new Allocation(allocation->category(), i == index ? amount : allocation->amount()));
  }
  setAllocationList(operation, allocations);
}

void OperationEditor::normalizeAllocations(Operation* operation) {
  if (!operation) return;
  QHash<const Category*, double> totals;
  QList<const Category*> categories;
  for (auto* allocation : operation->allocations()) {
    if (allocation && allocation->category() && std::abs(allocation->amount()) >= 0.001) {
      if (!totals.contains(allocation->category())) categories.append(allocation->category());
      totals[allocation->category()] += allocation->amount();
    }
  }
  QList<Allocation*> allocations;
  for (auto* category : categories)
    allocations.append(new Allocation(category, totals.value(category)));
  setAllocationList(operation, allocations);
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
