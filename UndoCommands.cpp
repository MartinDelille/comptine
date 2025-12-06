#include "UndoCommands.h"
#include "Account.h"
#include "AccountListModel.h"
#include "BudgetData.h"
#include "Category.h"
#include "OperationListModel.h"

RenameAccountCommand::RenameAccountCommand(Account *account,
                                           AccountListModel *accountModel,
                                           const QString &oldName,
                                           const QString &newName,
                                           QUndoCommand *parent) :
    QUndoCommand(parent), _account(account), _accountModel(accountModel), _oldName(oldName), _newName(newName) {
  setText(QObject::tr("Rename account to \"%1\"").arg(newName));
}

void RenameAccountCommand::undo() {
  if (_account) {
    _account->set_name(_oldName);
    if (_accountModel) {
      _accountModel->refresh();
    }
  }
}

void RenameAccountCommand::redo() {
  if (_account) {
    _account->set_name(_newName);
    if (_accountModel) {
      _accountModel->refresh();
    }
  }
}

ImportOperationsCommand::ImportOperationsCommand(Account *account,
                                                 OperationListModel *operationModel,
                                                 BudgetData *budgetData,
                                                 const QList<Operation *> &operations,
                                                 const QList<Category *> &newCategories,
                                                 QUndoCommand *parent) :
    QUndoCommand(parent),
    _account(account),
    _operationModel(operationModel),
    _budgetData(budgetData),
    _operations(operations),
    _newCategories(newCategories),
    _ownsOperations(false),
    _ownsCategories(false) {
  if (newCategories.isEmpty()) {
    setText(QObject::tr("Import %n operation(s)", "", operations.size()));
  } else {
    setText(QObject::tr("Import %n operation(s) with %1 new category(ies)", "", operations.size())
                .arg(newCategories.size()));
  }
}

ImportOperationsCommand::~ImportOperationsCommand() {
  // If we own the operations (they were undone), delete them
  if (_ownsOperations) {
    qDeleteAll(_operations);
  }
  // If we own the categories (they were undone), delete them
  if (_ownsCategories) {
    qDeleteAll(_newCategories);
  }
}

void ImportOperationsCommand::undo() {
  if (!_account) return;

  // Remove operations from account (don't delete them, we keep ownership)
  for (Operation *op : _operations) {
    _account->removeOperation(op);
  }
  _ownsOperations = true;

  // Remove new categories from budgetData (take ownership back, don't delete)
  if (_budgetData) {
    for (Category *cat : _newCategories) {
      _budgetData->takeCategoryByName(cat->name());
    }
  }
  _ownsCategories = true;

  // Refresh the model if it's showing this account
  if (_operationModel) {
    _operationModel->refresh();
  }
}

void ImportOperationsCommand::redo() {
  if (!_account) return;

  // Re-add operations to account
  for (Operation *op : _operations) {
    _account->addOperation(op);
  }
  _ownsOperations = false;

  // Re-add categories to budgetData
  if (_budgetData) {
    for (Category *cat : _newCategories) {
      _budgetData->addCategory(cat);
    }
  }
  _ownsCategories = false;

  // Refresh the model if it's showing this account
  if (_operationModel) {
    _operationModel->refresh();
  }
}
