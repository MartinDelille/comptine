#include "UndoCommands.h"
#include "Account.h"
#include "AccountListModel.h"
#include "BudgetData.h"
#include "Category.h"
#include "CategoryController.h"
#include "Operation.h"
#include "OperationListModel.h"

// AddAccountCommand implementation

AddAccountCommand::AddAccountCommand(Account* account, BudgetData* budgetData,
                                     QUndoCommand* parent) :
    QUndoCommand(parent),
    _account(account),
    _budgetData(budgetData),
    _ownsAccount(true) {  // We own the account until first redo
  setText(QObject::tr("Add account \"%1\"").arg(account->name()));
}

AddAccountCommand::~AddAccountCommand() {
  if (_ownsAccount) {
    delete _account;
  }
}

void AddAccountCommand::undo() {
  if (_budgetData) {
    _budgetData->takeAccount(_account);
    _ownsAccount = true;
    _budgetData->accountModel()->refresh();
  }
}

void AddAccountCommand::redo() {
  if (_budgetData) {
    _budgetData->addAccount(_account);
    _ownsAccount = false;

    // Select the newly added account
    int accountIndex = _budgetData->accounts().indexOf(_account);
    if (accountIndex >= 0) {
      _budgetData->selectAccount(accountIndex);
    }
    _budgetData->accountModel()->refresh();
  }
}

// RenameAccountCommand implementation

RenameAccountCommand::RenameAccountCommand(Account& account,
                                           AccountListModel* accountModel,
                                           const QString& oldName,
                                           const QString& newName,
                                           QUndoCommand* parent) :
    QUndoCommand(parent),
    _account(account),
    _accountModel(accountModel),
    _oldName(oldName),
    _newName(newName) {
  setText(QObject::tr("Rename account to \"%1\"").arg(newName));
}

void RenameAccountCommand::undo() {
  _account.set_name(_oldName);
  if (_accountModel) {
    _accountModel->refresh();
  }
}

void RenameAccountCommand::redo() {
  _account.set_name(_newName);
  if (_accountModel) {
    _accountModel->refresh();
  }
}

EditCategoryCommand::EditCategoryCommand(Category& category,
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
  // Rename operations back to old category name
  if (_oldName != _newName) {
    renameOperationsCategory(_newName, _oldName);
  }

  _category.set_name(_oldName);
  _category.set_budgetLimit(_oldBudgetLimit);
  if (_categoryController) {
    emit _categoryController->categoryCountChanged();  // Trigger UI refresh
  }
}

void EditCategoryCommand::redo() {
  _category.set_name(_newName);
  _category.set_budgetLimit(_newBudgetLimit);

  // Rename operations to new category name
  if (_oldName != _newName) {
    renameOperationsCategory(_oldName, _newName);
  }

  if (_categoryController) {
    emit _categoryController->categoryCountChanged();  // Trigger UI refresh
  }
}

// AddCategoriesCommand implementation

AddCategoriesCommand::AddCategoriesCommand(CategoryController* categoryController,
                                           const QList<Category*>& categories,
                                           QUndoCommand* parent) :
    QUndoCommand(parent),
    _categoryController(categoryController),
    _categories(categories),
    _ownsCategories(true) {  // We own the categories until first redo
  setText(QObject::tr("Add %n category(ies)", "", categories.size()));
}

AddCategoriesCommand::~AddCategoriesCommand() {
  if (_ownsCategories) {
    qDeleteAll(_categories);
  }
}

void AddCategoriesCommand::undo() {
  if (_categoryController) {
    for (Category* cat : _categories) {
      _categoryController->takeCategoryByName(cat->name());
    }
  }
  _ownsCategories = true;
}

void AddCategoriesCommand::redo() {
  if (_categoryController) {
    for (Category* cat : _categories) {
      _categoryController->addCategory(cat);
    }
  }
  _ownsCategories = false;
}

// ImportOperationsCommand implementation

ImportOperationsCommand::ImportOperationsCommand(Account* account,
                                                 OperationListModel* operationModel,
                                                 const QList<Operation*>& operations,
                                                 QUndoCommand* parent) :
    QUndoCommand(parent),
    _account(account),
    _operationModel(operationModel),
    _operations(operations),
    _ownsOperations(false) {
  setText(QObject::tr("Import %n operation(s)", "", operations.size()));
}

ImportOperationsCommand::~ImportOperationsCommand() {
  if (_ownsOperations) {
    qDeleteAll(_operations);
  }
}

void ImportOperationsCommand::undo() {
  // Remove operations from account and detach Qt parent to prevent double-delete
  // (when AddAccountCommand deletes the account, it would also delete child operations)
  for (Operation* op : _operations) {
    _account->removeOperation(op);
    op->setParent(nullptr);
  }
  _ownsOperations = true;

  if (_operationModel) {
    _operationModel->refresh();
  }
}

void ImportOperationsCommand::redo() {
  // Re-add operations to account
  for (Operation* op : _operations) {
    _account->addOperation(op);
  }
  _ownsOperations = false;

  if (_operationModel) {
    _operationModel->refresh();
  }
}

SetOperationCategoryCommand::SetOperationCategoryCommand(Operation& operation,
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
  _operation.set_category(_oldCategory);
  if (_operationModel) {
    _operationModel->refresh();
    _operationModel->selectByPointer(&_operation);
    emit _operationModel->operationDataChanged();
  }
}

void SetOperationCategoryCommand::redo() {
  _operation.set_category(_newCategory);
  if (_operationModel) {
    _operationModel->refresh();
    _operationModel->selectByPointer(&_operation);
    emit _operationModel->operationDataChanged();
  }
}

SetOperationBudgetDateCommand::SetOperationBudgetDateCommand(Operation& operation,
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
  _operation.set_budgetDate(_oldBudgetDate);
  if (_operationModel) {
    _operationModel->refresh();
    _operationModel->selectByPointer(&_operation);
    emit _operationModel->operationDataChanged();
  }
}

void SetOperationBudgetDateCommand::redo() {
  _operation.set_budgetDate(_newBudgetDate);
  if (_operationModel) {
    _operationModel->refresh();
    _operationModel->selectByPointer(&_operation);
    emit _operationModel->operationDataChanged();
  }
}

SplitOperationCommand::SplitOperationCommand(Operation& operation,
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
  if (_oldAllocations.isEmpty()) {
    // Was a single category, restore it
    _operation.clearAllocations();
    _operation.set_category(_oldCategory);
  } else {
    // Was already split, restore old allocations
    _operation.setAllocations(_oldAllocations);
  }
  if (_operationModel) {
    _operationModel->refresh();
    _operationModel->selectByPointer(&_operation);
    emit _operationModel->operationDataChanged();
  }
}

void SplitOperationCommand::redo() {
  if (_newAllocations.size() == 1) {
    // Single category - use regular category field
    _operation.clearAllocations();
    _operation.set_category(_newAllocations.first().category);
  } else if (_newAllocations.isEmpty()) {
    // Clear everything
    _operation.clearAllocations();
    _operation.set_category(QString());
  } else {
    // Multiple categories - use allocations
    _operation.setAllocations(_newAllocations);
  }
  if (_operationModel) {
    _operationModel->refresh();
    _operationModel->selectByPointer(&_operation);
    emit _operationModel->operationDataChanged();
  }
}

SetOperationAmountCommand::SetOperationAmountCommand(Operation& operation,
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
  _operation.set_amount(_oldAmount);
  if (_operationModel) {
    _operationModel->refresh();
    _operationModel->selectByPointer(&_operation);
    emit _operationModel->operationDataChanged();
  }
}

void SetOperationAmountCommand::redo() {
  _operation.set_amount(_newAmount);
  if (_operationModel) {
    _operationModel->refresh();
    _operationModel->selectByPointer(&_operation);
    emit _operationModel->operationDataChanged();
  }
}

SetOperationDateCommand::SetOperationDateCommand(Operation& operation,
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
  _operation.set_date(_oldDate);
  if (_operationModel) {
    if (_operationModel->account()) {
      _operationModel->account()->sortOperations();
    }
    _operationModel->refresh();
    _operationModel->selectByPointer(&_operation);
    emit _operationModel->operationDataChanged();
  }
}

void SetOperationDateCommand::redo() {
  _operation.set_date(_newDate);
  if (_operationModel) {
    if (_operationModel->account()) {
      _operationModel->account()->sortOperations();
    }
    _operationModel->refresh();
    _operationModel->selectByPointer(&_operation);
    emit _operationModel->operationDataChanged();
  }
}

SetOperationDescriptionCommand::SetOperationDescriptionCommand(Operation& operation,
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
  _operation.set_description(_oldDescription);
  if (_operationModel) {
    _operationModel->refresh();
    _operationModel->selectByPointer(&_operation);
    emit _operationModel->operationDataChanged();
  }
}

void SetOperationDescriptionCommand::redo() {
  _operation.set_description(_newDescription);
  if (_operationModel) {
    _operationModel->refresh();
    _operationModel->selectByPointer(&_operation);
    emit _operationModel->operationDataChanged();
  }
}

// SetLeftoverDecisionCommand implementation

SetLeftoverDecisionCommand::SetLeftoverDecisionCommand(Category& category,
                                                       CategoryController* categoryController,
                                                       int year, int month,
                                                       const LeftoverDecision& oldDecision,
                                                       const LeftoverDecision& newDecision,
                                                       QUndoCommand* parent) :
    QUndoCommand(parent),
    _category(category),
    _categoryController(categoryController),
    _year(year),
    _month(month),
    _oldDecision(oldDecision),
    _newDecision(newDecision) {
  QString actionStr;
  if (newDecision.saveAmount > 0 && newDecision.reportAmount > 0) {
    actionStr = QObject::tr("save %1 and report %2")
                    .arg(newDecision.saveAmount, 0, 'f', 2)
                    .arg(newDecision.reportAmount, 0, 'f', 2);
  } else if (newDecision.saveAmount > 0) {
    actionStr = QObject::tr("save %1").arg(newDecision.saveAmount, 0, 'f', 2);
  } else if (newDecision.reportAmount != 0) {
    actionStr = QObject::tr("report %1").arg(newDecision.reportAmount, 0, 'f', 2);
  } else {
    actionStr = QObject::tr("clear");
  }
  setText(QObject::tr("Set leftover for \"%1\" to %2").arg(category.name(), actionStr));
}

void SetLeftoverDecisionCommand::undo() {
  if (_oldDecision.isEmpty()) {
    _category.clearLeftoverDecision(_year, _month);
  } else {
    _category.setLeftoverDecision(_year, _month, _oldDecision);
  }
  if (_categoryController) {
    emit _categoryController->leftoverDataChanged();
  }
}

void SetLeftoverDecisionCommand::redo() {
  if (_newDecision.isEmpty()) {
    _category.clearLeftoverDecision(_year, _month);
  } else {
    _category.setLeftoverDecision(_year, _month, _newDecision);
  }
  if (_categoryController) {
    emit _categoryController->leftoverDataChanged();
  }
}

int SetLeftoverDecisionCommand::id() const {
  // Unique ID for leftover decision commands to enable merging
  return 1001;
}

bool SetLeftoverDecisionCommand::mergeWith(const QUndoCommand* other) {
  const SetLeftoverDecisionCommand* cmd = dynamic_cast<const SetLeftoverDecisionCommand*>(other);
  if (!cmd)
    return false;

  // Only merge if same category, year, month
  if (&cmd->_category != &_category || cmd->_year != _year || cmd->_month != _month)
    return false;

  // Keep our old decision (for undo), take their new decision (for redo)
  _newDecision = cmd->_newDecision;

  // Update the command text to reflect the final state
  QString actionStr;
  if (_newDecision.saveAmount > 0 && _newDecision.reportAmount > 0) {
    actionStr = QObject::tr("save %1 and report %2")
                    .arg(_newDecision.saveAmount, 0, 'f', 2)
                    .arg(_newDecision.reportAmount, 0, 'f', 2);
  } else if (_newDecision.saveAmount > 0) {
    actionStr = QObject::tr("save %1").arg(_newDecision.saveAmount, 0, 'f', 2);
  } else if (_newDecision.reportAmount != 0) {
    actionStr = QObject::tr("report %1").arg(_newDecision.reportAmount, 0, 'f', 2);
  } else {
    actionStr = QObject::tr("clear");
  }
  setText(QObject::tr("Set leftover for \"%1\" to %2").arg(_category.name(), actionStr));

  return true;
}
