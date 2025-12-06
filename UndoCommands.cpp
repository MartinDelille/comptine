#include "UndoCommands.h"
#include "Account.h"
#include "AccountListModel.h"
#include "BudgetData.h"
#include "Category.h"
#include "Operation.h"
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

EditCategoryCommand::EditCategoryCommand(Category *category,
                                         BudgetData *budgetData,
                                         const QString &oldName,
                                         const QString &newName,
                                         double oldBudgetLimit,
                                         double newBudgetLimit,
                                         QUndoCommand *parent) :
    QUndoCommand(parent),
    _category(category),
    _budgetData(budgetData),
    _oldName(oldName),
    _newName(newName),
    _oldBudgetLimit(oldBudgetLimit),
    _newBudgetLimit(newBudgetLimit) {
  if (oldName != newName && oldBudgetLimit != newBudgetLimit) {
    setText(QObject::tr("Edit category \"%1\"").arg(newName));
  } else if (oldName != newName) {
    setText(QObject::tr("Rename category to \"%1\"").arg(newName));
  } else {
    setText(QObject::tr("Change budget limit of \"%1\"").arg(newName));
  }
}

void EditCategoryCommand::renameOperationsCategory(const QString &fromName, const QString &toName) {
  if (!_budgetData || fromName == toName) return;

  // Update all operations that use this category
  for (Account *account : _budgetData->accounts()) {
    for (Operation *op : account->operations()) {
      if (op->category() == fromName) {
        op->set_category(toName);
      }
    }
  }
}

void EditCategoryCommand::undo() {
  if (_category) {
    // Rename operations back to old category name
    if (_oldName != _newName) {
      renameOperationsCategory(_newName, _oldName);
    }

    _category->set_name(_oldName);
    _category->set_budgetLimit(_oldBudgetLimit);
    if (_budgetData) {
      emit _budgetData->categoryCountChanged();  // Trigger UI refresh
    }
  }
}

void EditCategoryCommand::redo() {
  if (_category) {
    _category->set_name(_newName);
    _category->set_budgetLimit(_newBudgetLimit);

    // Rename operations to new category name
    if (_oldName != _newName) {
      renameOperationsCategory(_oldName, _newName);
    }

    if (_budgetData) {
      emit _budgetData->categoryCountChanged();  // Trigger UI refresh
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

SetOperationCategoryCommand::SetOperationCategoryCommand(Operation *operation,
                                                         OperationListModel *operationModel,
                                                         BudgetData *budgetData,
                                                         const QString &oldCategory,
                                                         const QString &newCategory,
                                                         QUndoCommand *parent) :
    QUndoCommand(parent),
    _operation(operation),
    _operationModel(operationModel),
    _budgetData(budgetData),
    _oldCategory(oldCategory),
    _newCategory(newCategory) {
  if (newCategory.isEmpty()) {
    setText(QObject::tr("Clear operation category"));
  } else {
    setText(QObject::tr("Set operation category to \"%1\"").arg(newCategory));
  }
}

void SetOperationCategoryCommand::undo() {
  if (_operation) {
    _operation->set_category(_oldCategory);
    if (_operationModel) {
      _operationModel->refresh();
    }
    if (_budgetData) {
      emit _budgetData->operationDataChanged();
    }
  }
}

void SetOperationCategoryCommand::redo() {
  if (_operation) {
    _operation->set_category(_newCategory);
    if (_operationModel) {
      _operationModel->refresh();
    }
    if (_budgetData) {
      emit _budgetData->operationDataChanged();
    }
  }
}

SetOperationBudgetDateCommand::SetOperationBudgetDateCommand(Operation *operation,
                                                             OperationListModel *operationModel,
                                                             BudgetData *budgetData,
                                                             const QDate &oldBudgetDate,
                                                             const QDate &newBudgetDate,
                                                             QUndoCommand *parent) :
    QUndoCommand(parent),
    _operation(operation),
    _operationModel(operationModel),
    _budgetData(budgetData),
    _oldBudgetDate(oldBudgetDate),
    _newBudgetDate(newBudgetDate) {
  setText(QObject::tr("Set operation budget date to %1").arg(newBudgetDate.toString("dd/MM/yyyy")));
}

void SetOperationBudgetDateCommand::undo() {
  if (_operation) {
    _operation->set_budgetDate(_oldBudgetDate);
    if (_operationModel) {
      _operationModel->refresh();
    }
    if (_budgetData) {
      emit _budgetData->operationDataChanged();
    }
  }
}

void SetOperationBudgetDateCommand::redo() {
  if (_operation) {
    _operation->set_budgetDate(_newBudgetDate);
    if (_operationModel) {
      _operationModel->refresh();
    }
    if (_budgetData) {
      emit _budgetData->operationDataChanged();
    }
  }
}
