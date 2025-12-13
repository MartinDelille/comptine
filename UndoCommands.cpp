#include "UndoCommands.h"
#include "Account.h"
#include "AccountListModel.h"
#include "BudgetData.h"
#include "Category.h"
#include "CategoryController.h"
#include "Operation.h"
#include "OperationListModel.h"

RenameAccountCommand::RenameAccountCommand(Account* account,
                                           AccountListModel* accountModel,
                                           const QString& oldName,
                                           const QString& newName,
                                           QUndoCommand* parent) :
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

EditCategoryCommand::EditCategoryCommand(Category* category,
                                         BudgetData* budgetData,
                                         CategoryController* categoryController,
                                         const QString& oldName,
                                         const QString& newName,
                                         double oldBudgetLimit,
                                         double newBudgetLimit,
                                         QUndoCommand* parent) :
    QUndoCommand(parent),
    _category(category),
    _budgetData(budgetData),
    _categoryController(categoryController),
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

void EditCategoryCommand::renameOperationsCategory(const QString& fromName, const QString& toName) {
  if (!_budgetData || fromName == toName) return;

  // Update all operations that use this category
  for (Account* account : _budgetData->accounts()) {
    for (Operation* op : account->operations()) {
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
    if (_categoryController) {
      emit _categoryController->categoryCountChanged();  // Trigger UI refresh
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

    if (_categoryController) {
      emit _categoryController->categoryCountChanged();  // Trigger UI refresh
    }
  }
}

ImportOperationsCommand::ImportOperationsCommand(Account* account,
                                                 OperationListModel* operationModel,
                                                 CategoryController* categoryController,
                                                 const QList<Operation*>& operations,
                                                 const QList<Category*>& newCategories,
                                                 QUndoCommand* parent) :
    QUndoCommand(parent),
    _account(account),
    _operationModel(operationModel),
    _categoryController(categoryController),
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
  for (Operation* op : _operations) {
    _account->removeOperation(op);
  }
  _ownsOperations = true;

  // Remove new categories from CategoryController (take ownership back, don't delete)
  if (_categoryController) {
    for (Category* cat : _newCategories) {
      _categoryController->takeCategoryByName(cat->name());
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
  for (Operation* op : _operations) {
    _account->addOperation(op);
  }
  _ownsOperations = false;

  // Re-add categories to CategoryController
  if (_categoryController) {
    for (Category* cat : _newCategories) {
      _categoryController->addCategory(cat);
    }
  }
  _ownsCategories = false;

  // Refresh the model if it's showing this account
  if (_operationModel) {
    _operationModel->refresh();
  }
}

SetOperationCategoryCommand::SetOperationCategoryCommand(Operation* operation,
                                                         OperationListModel* operationModel,
                                                         const QString& oldCategory,
                                                         const QString& newCategory,
                                                         QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _operationModel(operationModel),
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
      _operationModel->selectByPointer(_operation);
      emit _operationModel->operationDataChanged();
    }
  }
}

void SetOperationCategoryCommand::redo() {
  if (_operation) {
    _operation->set_category(_newCategory);
    if (_operationModel) {
      _operationModel->refresh();
      _operationModel->selectByPointer(_operation);
      emit _operationModel->operationDataChanged();
    }
  }
}

SetOperationBudgetDateCommand::SetOperationBudgetDateCommand(Operation* operation,
                                                             OperationListModel* operationModel,
                                                             const QDate& oldBudgetDate,
                                                             const QDate& newBudgetDate,
                                                             QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _operationModel(operationModel),
    _oldBudgetDate(oldBudgetDate),
    _newBudgetDate(newBudgetDate) {
  setText(QObject::tr("Set operation budget date to %1").arg(newBudgetDate.toString("dd/MM/yyyy")));
}

void SetOperationBudgetDateCommand::undo() {
  if (_operation) {
    _operation->set_budgetDate(_oldBudgetDate);
    if (_operationModel) {
      _operationModel->refresh();
      _operationModel->selectByPointer(_operation);
      emit _operationModel->operationDataChanged();
    }
  }
}

void SetOperationBudgetDateCommand::redo() {
  if (_operation) {
    _operation->set_budgetDate(_newBudgetDate);
    if (_operationModel) {
      _operationModel->refresh();
      _operationModel->selectByPointer(_operation);
      emit _operationModel->operationDataChanged();
    }
  }
}

SplitOperationCommand::SplitOperationCommand(Operation* operation,
                                             OperationListModel* operationModel,
                                             const QString& oldCategory,
                                             const QList<CategoryAllocation>& oldAllocations,
                                             const QList<CategoryAllocation>& newAllocations,
                                             QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _operationModel(operationModel),
    _oldCategory(oldCategory),
    _oldAllocations(oldAllocations),
    _newAllocations(newAllocations) {
  if (newAllocations.size() > 1) {
    setText(QObject::tr("Split operation into %1 categories").arg(newAllocations.size()));
  } else if (newAllocations.size() == 1) {
    setText(QObject::tr("Set operation category to \"%1\"").arg(newAllocations.first().category));
  } else {
    setText(QObject::tr("Clear operation split"));
  }
}

void SplitOperationCommand::undo() {
  if (_operation) {
    if (_oldAllocations.isEmpty()) {
      // Was a single category, restore it
      _operation->clearAllocations();
      _operation->set_category(_oldCategory);
    } else {
      // Was already split, restore old allocations
      _operation->setAllocations(_oldAllocations);
    }
    if (_operationModel) {
      _operationModel->refresh();
      _operationModel->selectByPointer(_operation);
      emit _operationModel->operationDataChanged();
    }
  }
}

void SplitOperationCommand::redo() {
  if (_operation) {
    if (_newAllocations.size() == 1) {
      // Single category - use regular category field
      _operation->clearAllocations();
      _operation->set_category(_newAllocations.first().category);
    } else if (_newAllocations.isEmpty()) {
      // Clear everything
      _operation->clearAllocations();
      _operation->set_category(QString());
    } else {
      // Multiple categories - use allocations
      _operation->setAllocations(_newAllocations);
    }
    if (_operationModel) {
      _operationModel->refresh();
      _operationModel->selectByPointer(_operation);
      emit _operationModel->operationDataChanged();
    }
  }
}

SetOperationAmountCommand::SetOperationAmountCommand(Operation* operation,
                                                     OperationListModel* operationModel,
                                                     double oldAmount,
                                                     double newAmount,
                                                     QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _operationModel(operationModel),
    _oldAmount(oldAmount),
    _newAmount(newAmount) {
  setText(QObject::tr("Set operation amount to %1").arg(newAmount, 0, 'f', 2));
}

void SetOperationAmountCommand::undo() {
  if (_operation) {
    _operation->set_amount(_oldAmount);
    if (_operationModel) {
      _operationModel->refresh();
      _operationModel->selectByPointer(_operation);
      emit _operationModel->operationDataChanged();
    }
  }
}

void SetOperationAmountCommand::redo() {
  if (_operation) {
    _operation->set_amount(_newAmount);
    if (_operationModel) {
      _operationModel->refresh();
      _operationModel->selectByPointer(_operation);
      emit _operationModel->operationDataChanged();
    }
  }
}

SetOperationDateCommand::SetOperationDateCommand(Operation* operation,
                                                 OperationListModel* operationModel,
                                                 const QDate& oldDate,
                                                 const QDate& newDate,
                                                 QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _operationModel(operationModel),
    _oldDate(oldDate),
    _newDate(newDate) {
  setText(QObject::tr("Set operation date to %1").arg(newDate.toString("dd/MM/yyyy")));
}

void SetOperationDateCommand::undo() {
  if (_operation) {
    _operation->set_date(_oldDate);
    if (_operationModel) {
      if (_operationModel->account()) {
        _operationModel->account()->sortOperations();
      }
      _operationModel->refresh();
      _operationModel->selectByPointer(_operation);
      emit _operationModel->operationDataChanged();
    }
  }
}

void SetOperationDateCommand::redo() {
  if (_operation) {
    _operation->set_date(_newDate);
    if (_operationModel) {
      if (_operationModel->account()) {
        _operationModel->account()->sortOperations();
      }
      _operationModel->refresh();
      _operationModel->selectByPointer(_operation);
      emit _operationModel->operationDataChanged();
    }
  }
}

SetOperationDescriptionCommand::SetOperationDescriptionCommand(Operation* operation,
                                                               OperationListModel* operationModel,
                                                               const QString& oldDescription,
                                                               const QString& newDescription,
                                                               QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _operationModel(operationModel),
    _oldDescription(oldDescription),
    _newDescription(newDescription) {
  setText(QObject::tr("Set operation description"));
}

void SetOperationDescriptionCommand::undo() {
  if (_operation) {
    _operation->set_description(_oldDescription);
    if (_operationModel) {
      _operationModel->refresh();
      _operationModel->selectByPointer(_operation);
      emit _operationModel->operationDataChanged();
    }
  }
}

void SetOperationDescriptionCommand::redo() {
  if (_operation) {
    _operation->set_description(_newDescription);
    if (_operationModel) {
      _operationModel->refresh();
      _operationModel->selectByPointer(_operation);
      emit _operationModel->operationDataChanged();
    }
  }
}
