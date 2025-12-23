#include "UndoCommands.h"
#include "Account.h"
#include "AccountListModel.h"
#include "BudgetData.h"
#include "CategorizationRule.h"
#include "Category.h"
#include "CategoryController.h"
#include "Operation.h"
#include "OperationListModel.h"
#include "RuleController.h"
#include "RuleListModel.h"

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
                                         const QString& oldName,
                                         const QString& newName,
                                         double oldBudgetLimit,
                                         double newBudgetLimit,
                                         QUndoCommand* parent) :
    QUndoCommand(parent),
    _category(category),
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

void EditCategoryCommand::undo() {
  _category.set_name(_oldName);
  _category.set_budgetLimit(_oldBudgetLimit);
}

void EditCategoryCommand::redo() {
  _category.set_name(_newName);
  _category.set_budgetLimit(_newBudgetLimit);
}

// AddCategoryCommand implementation

AddCategoryCommand::AddCategoryCommand(CategoryController* categoryController, Category* category,
                                       QUndoCommand* parent) :
    QUndoCommand(parent),
    _categoryController(categoryController),
    _category(category),
    _ownsCategory(true) {
  setText(QObject::tr("Add category \"%1\"").arg(category->name()));
}

AddCategoryCommand::~AddCategoryCommand() {
  if (_ownsCategory) {
    delete _category;
  }
}

void AddCategoryCommand::undo() {
  if (_categoryController) {
    _categoryController->takeCategoryByName(_category->name());
  }
  _ownsCategory = true;
}

void AddCategoryCommand::redo() {
  if (_categoryController) {
    _categoryController->addCategory(_category);
  }
  _ownsCategory = false;
}

// AddOperationCommand implementation

ImportOperationsCommand::ImportOperationsCommand(Account& account,
                                                 OperationListModel& operationModel,
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
    _account.removeOperation(op);
    op->setParent(nullptr);
  }
  _ownsOperations = true;

  _operationModel.refresh();
}

void ImportOperationsCommand::redo() {
  // Re-add operations to account
  for (Operation* op : _operations) {
    _account.addOperation(op);
  }
  _ownsOperations = false;
  _operationModel.refresh();
}

AddOperationCommand::AddOperationCommand(Operation* operation,
                                         Account& account,
                                         OperationListModel& operationModel,
                                         QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _account(account),
    _operationModel(operationModel) {
  setText(QObject::tr("Add operation: \"%0\"").arg(_operation->label()));
}

void AddOperationCommand::undo() {
  _account.removeOperation(_operation);
  _operation->setParent(nullptr);
  _operationModel.refresh();
}

void AddOperationCommand::redo() {
  _account.addOperation(_operation);
  _operationModel.refresh();
  _operationModel.selectByPointer(_operation);
  emit _operationModel.operationDataChanged();
}

SetOperationCategoryCommand::SetOperationCategoryCommand(Operation& operation,
                                                         OperationListModel* operationModel,
                                                         const Category* oldCategory,
                                                         const Category* newCategory,
                                                         QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _operationModel(operationModel),
    _oldCategory(oldCategory),
    _newCategory(newCategory) {
  if (newCategory) {
    setText(QObject::tr("Set operation category to \"%1\"").arg(newCategory->name()));
  } else {
    setText(QObject::tr("Clear operation category"));
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
                                             const Category* oldCategory,
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
    auto category = newAllocations.first().category;
    auto name = category ? category->name() : "";
    setText(QObject::tr("Set operation category to \"%1\"").arg(name));
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
    _operation.set_category(nullptr);
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

SetOperationLabelCommand::SetOperationLabelCommand(Operation& operation,
                                                   OperationListModel* operationModel,
                                                   const QString& oldLabel,
                                                   const QString& newLabel,
                                                   QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _operationModel(operationModel),
    _oldLabel(oldLabel),
    _newLabel(newLabel) {
  setText(QObject::tr("Set operation label"));
}

void SetOperationLabelCommand::undo() {
  _operation.set_label(_oldLabel);
  if (_operationModel) {
    _operationModel->refresh();
    _operationModel->selectByPointer(&_operation);
    emit _operationModel->operationDataChanged();
  }
}

void SetOperationLabelCommand::redo() {
  _operation.set_label(_newLabel);
  if (_operationModel) {
    _operationModel->refresh();
    _operationModel->selectByPointer(&_operation);
    emit _operationModel->operationDataChanged();
  }
}

SetOperationDetailsCommand::SetOperationDetailsCommand(Operation& operation,
                                                       OperationListModel* operationModel,
                                                       const QString& oldDetails,
                                                       const QString& newDetails,
                                                       QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _operationModel(operationModel),
    _oldDetails(oldDetails),
    _newDetails(newDetails) {
  setText(QObject::tr("Set operation details"));
}

void SetOperationDetailsCommand::undo() {
  _operation.set_details(_oldDetails);
  if (_operationModel) {
    _operationModel->refresh();
    _operationModel->selectByPointer(&_operation);
    emit _operationModel->operationDataChanged();
  }
}

void SetOperationDetailsCommand::redo() {
  _operation.set_details(_newDetails);
  if (_operationModel) {
    _operationModel->refresh();
    _operationModel->selectByPointer(&_operation);
    emit _operationModel->operationDataChanged();
  }
}

// SetLeftoverDecisionCommand implementation

SetLeftoverDecisionCommand::SetLeftoverDecisionCommand(Category& category,
                                                       CategoryController* categoryController,
                                                       const QDate& date,
                                                       const LeftoverDecision& oldDecision,
                                                       const LeftoverDecision& newDecision,
                                                       QUndoCommand* parent) :
    QUndoCommand(parent),
    _category(category),
    _categoryController(categoryController),
    _date(date),
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
    _category.clearLeftoverDecision(_date.year(), _date.month());
  } else {
    _category.setLeftoverDecision(_date.year(), _date.month(), _oldDecision);
  }
  if (_categoryController) {
    emit _categoryController->leftoverDataChanged();
  }
}

void SetLeftoverDecisionCommand::redo() {
  if (_newDecision.isEmpty()) {
    _category.clearLeftoverDecision(_date.year(), _date.month());
  } else {
    _category.setLeftoverDecision(_date.year(), _date.month(), _newDecision);
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
  if (&cmd->_category != &_category || cmd->_date != _date)
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

// AddRuleCommand implementation

AddRuleCommand::AddRuleCommand(RuleController* ruleController, CategorizationRule* rule,
                               QUndoCommand* parent) :
    QUndoCommand(parent),
    _ruleController(ruleController),
    _rule(rule),
    _ownsRule(true) {
  setText(QObject::tr("Add rule for \"%1\"").arg(rule->labelPrefix()));
}

AddRuleCommand::~AddRuleCommand() {
  if (_ownsRule) {
    delete _rule;
  }
}

void AddRuleCommand::undo() {
  if (_ruleController) {
    // Find and remove the rule
    int index = _ruleController->rules().indexOf(_rule);
    if (index >= 0) {
      _ruleController->takeRule(index);
      _ownsRule = true;
    }
  }
}

void AddRuleCommand::redo() {
  if (_ruleController) {
    _ruleController->addRule(_rule);
    _ownsRule = false;
  }
}

// RemoveRuleCommand implementation

RemoveRuleCommand::RemoveRuleCommand(RuleController* ruleController, int index,
                                     QUndoCommand* parent) :
    QUndoCommand(parent),
    _ruleController(ruleController),
    _rule(nullptr),
    _index(index),
    _ownsRule(false) {
  if (ruleController && index >= 0 && index < ruleController->rules().size()) {
    _rule = ruleController->rules().at(index);
    setText(QObject::tr("Remove rule for \"%1\"").arg(_rule->labelPrefix()));
  }
}

RemoveRuleCommand::~RemoveRuleCommand() {
  if (_ownsRule) {
    delete _rule;
  }
}

void RemoveRuleCommand::undo() {
  if (_ruleController && _rule) {
    // Re-insert the rule at the original index
    _ruleController->addRule(_rule);
    // Move it to the original position if needed
    int currentIndex = _ruleController->rules().indexOf(_rule);
    if (currentIndex != _index && currentIndex >= 0) {
      _ruleController->moveRule(currentIndex, _index);
    }
    _ownsRule = false;
  }
}

void RemoveRuleCommand::redo() {
  if (_ruleController && _rule) {
    int index = _ruleController->rules().indexOf(_rule);
    if (index >= 0) {
      _rule = _ruleController->takeRule(index);
      _ownsRule = true;
    }
  }
}

// EditRuleCommand implementation

EditRuleCommand::EditRuleCommand(RuleController* ruleController, int index,
                                 const Category* oldCategory, const Category* newCategory,
                                 const QString& oldLabelPrefix, const QString& newLabelPrefix,
                                 QUndoCommand* parent) :
    QUndoCommand(parent),
    _ruleController(ruleController),
    _index(index),
    _oldCategory(oldCategory),
    _newCategory(newCategory),
    _oldLabelPrefix(oldLabelPrefix),
    _newLabelPrefix(newLabelPrefix) {
  setText(QObject::tr("Edit rule for \"%1\"").arg(newLabelPrefix));
}

void EditRuleCommand::undo() {
  if (_ruleController) {
    CategorizationRule* rule = _ruleController->getRule(_index);
    if (rule) {
      rule->set_category(_oldCategory);
      rule->set_labelPrefix(_oldLabelPrefix);
      _ruleController->ruleModel()->refresh();
      emit _ruleController->rulesChanged();
    }
  }
}

void EditRuleCommand::redo() {
  if (_ruleController) {
    CategorizationRule* rule = _ruleController->getRule(_index);
    if (rule) {
      rule->set_category(_newCategory);
      rule->set_labelPrefix(_newLabelPrefix);
      _ruleController->ruleModel()->refresh();
      emit _ruleController->rulesChanged();
    }
  }
}

// MoveRuleCommand implementation
MoveRuleCommand::MoveRuleCommand(RuleController* ruleController, int fromIndex, int toIndex,
                                 QUndoCommand* parent) :
    QUndoCommand(parent),
    _ruleController(ruleController),
    _fromIndex(fromIndex),
    _toIndex(toIndex) {
  setText(QObject::tr("Move rule"));
}

void MoveRuleCommand::undo() {
  if (_ruleController) {
    _ruleController->moveRuleDirect(_toIndex, _fromIndex);
  }
}

void MoveRuleCommand::redo() {
  if (_ruleController) {
    _ruleController->moveRuleDirect(_fromIndex, _toIndex);
  }
}
