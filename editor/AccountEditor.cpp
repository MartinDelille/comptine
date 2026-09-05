#include "AccountEditor.h"

#include "BudgetData.h"
#include "UndoCommands.h"

AccountEditor::AccountEditor(BudgetData& budgetData, QUndoStack& undoStack, QObject* parent) :
    QObject(parent), _budgetData(budgetData), _undoStack(undoStack) {
}

void AccountEditor::renameCurrentAccount(const QString& name) {
  auto* account = _budgetData.currentAccount();
  if (!account || name.isEmpty() || account->name() == name) return;
  _undoStack.push(new RenameAccountCommand(*account, name));
}
