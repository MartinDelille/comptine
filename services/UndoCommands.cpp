#include "UndoCommands.h"
#include "BudgetData.h"
#include "CategoryController.h"
#include "RuleController.h"
#include "RuleListModel.h"
#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "model/Rule.h"

// AddAccountCommand implementation

AddAccountCommand::AddAccountCommand(Account* account, BudgetData& budgetData,
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
  _budgetData.takeAccount(_account);
  _ownsAccount = true;
}

void AddAccountCommand::redo() {
  _budgetData.addAccount(_account);
  _ownsAccount = false;

  // Select the newly added account
  _budgetData.set_currentAccount(_account);
}

// RenameAccountCommand implementation

RenameAccountCommand::RenameAccountCommand(Account& account,
                                           const QString& newName,
                                           QUndoCommand* parent) :
    QUndoCommand(parent),
    _account(account),
    _oldName(account.name()),
    _newName(newName) {
  setText(QObject::tr("Rename account to \"%1\"").arg(newName));
}

void RenameAccountCommand::undo() {
  _account.set_name(_oldName);
}

void RenameAccountCommand::redo() {
  _account.set_name(_newName);
}

EditCategoryCommand::EditCategoryCommand(Category& category,
                                         const QString& newName,
                                         double newBudgetLimit,
                                         const QDate& budgetDate,
                                         QUndoCommand* parent) :
    QUndoCommand(parent),
    _category(category),
    _oldName(category.name()),
    _newName(newName),
    _oldBudgetLimit(category.budgetLimitForMonth(budgetDate)),
    _newBudgetLimit(newBudgetLimit),
    _budgetDate(budgetDate),
    _historyDate(budgetDate.addMonths(-1)) {
  // Save the existing month_history budget limit for the previous month (to restore on undo)
  MonthRecord existingRecord = category.monthRecord(_historyDate.year(), _historyDate.month());
  _previousHistoryBudgetLimit = existingRecord.budgetLimit;

  if (_oldName != _newName && _oldBudgetLimit != _newBudgetLimit) {
    setText(QObject::tr("Edit category \"%1\"").arg(newName));
  } else if (_oldName != _newName) {
    setText(QObject::tr("Rename category to \"%1\"").arg(newName));
  } else {
    setText(QObject::tr("Change budget limit of \"%1\"").arg(newName));
  }
}

void EditCategoryCommand::undo() {
  _category.set_name(_oldName);

  // If the budget limit changed, restore month_history and category budgetLimit
  if (_oldBudgetLimit != _newBudgetLimit) {
    // Restore the previous month_history entry for the month before budgetDate
    if (_previousHistoryBudgetLimit.has_value()) {
      _category.setBudgetLimitForMonth(_historyDate.year(), _historyDate.month(), _previousHistoryBudgetLimit.value());
    } else {
      _category.clearBudgetLimitForMonth(_historyDate.year(), _historyDate.month());
    }
    // Restore the old category budget limit
    _category.set_budgetLimit(_oldBudgetLimit);
  }
}

void EditCategoryCommand::redo() {
  _category.set_name(_newName);

  // If the budget limit changed, record the old limit in month_history for the month before budgetDate
  if (_oldBudgetLimit != _newBudgetLimit) {
    // Record the old budget limit as the effective limit for the month before budgetDate and earlier
    _category.setBudgetLimitForMonth(_historyDate.year(), _historyDate.month(), _oldBudgetLimit);
    // Set the new category budget limit (effective for budgetDate and after)
    _category.set_budgetLimit(_newBudgetLimit);
  }
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

// DeleteCategoryCommand implementation

DeleteCategoryCommand::DeleteCategoryCommand(
    CategoryController* categoryController,
    Category* category,
    QUndoCommand* parent) :
    QUndoCommand(parent),
    _categoryController(categoryController),
    _category(category),
    _ownsCategory(true) {
  setText(QObject::tr("Delete category \"%0\"").arg(category->name()));
}

DeleteCategoryCommand::~DeleteCategoryCommand() {
  if (_ownsCategory) {
    delete _category;
  }
}

void DeleteCategoryCommand::undo() {
  if (_categoryController) {
    _categoryController->addCategory(_category);
  }
  _ownsCategory = false;
}

void DeleteCategoryCommand::redo() {
  if (_categoryController) {
    _categoryController->takeCategoryByName(_category->name());
  }
  _ownsCategory = true;
}

// AddOperationCommand implementation

ImportOperationsCommand::ImportOperationsCommand(Account& account,
                                                 const QList<Operation*>& operations,
                                                 QUndoCommand* parent) :
    QUndoCommand(parent),
    _account(account),
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
  for (auto op : _operations) {
    _account.removeOperation(op);
  }
  _ownsOperations = true;

  _account.refresh();
}

void ImportOperationsCommand::redo() {
  // Re-add operations to account
  for (auto op : _operations) {
    _account.addOperation(op);
  }
  _ownsOperations = false;
  _account.refresh();
}

AddOperationCommand::AddOperationCommand(Operation* operation,
                                         Account& account,
                                         QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _account(account) {
  setText(QObject::tr("Add operation: \"%0\"").arg(_operation->label()));
}

AddOperationCommand::~AddOperationCommand() {
  if (_ownsOperation) delete _operation;
}

void AddOperationCommand::undo() {
  _account.removeOperation(_operation);
  _ownsOperation = true;
  _account.refresh();
}

void AddOperationCommand::redo() {
  _account.addOperation(_operation);
  _ownsOperation = false;
}

DeleteOperationCommand::DeleteOperationCommand(Operation* operation,
                                               Account& account,
                                               QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _account(account) {
  setText(QObject::tr("Add operation: \"%0\"").arg(_operation->label()));
}

DeleteOperationCommand::~DeleteOperationCommand() {
  if (_ownsOperation) delete _operation;
}

void DeleteOperationCommand::undo() {
  _account.addOperation(_operation);
  _ownsOperation = false;
}

void DeleteOperationCommand::redo() {
  _account.removeOperation(_operation);
  _ownsOperation = true;
  _account.refresh();
}

SetOperationBudgetDateCommand::SetOperationBudgetDateCommand(Operation& operation,
                                                             const QDate& newBudgetDate,
                                                             QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _oldBudgetDate(operation.budgetDate()),
    _newBudgetDate(newBudgetDate) {
  setText(QObject::tr("Set operation budget date to %1").arg(newBudgetDate.toString("dd/MM/yyyy")));
}

void SetOperationBudgetDateCommand::undo() {
  _operation.set_budgetDate(_oldBudgetDate);
}

void SetOperationBudgetDateCommand::redo() {
  _operation.set_budgetDate(_newBudgetDate);
}

SplitOperationCommand::SplitOperationCommand(Operation& operation,
                                             const QList<Allocation*>& newAllocations,
                                             QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _oldAllocations(cloneAllocations(operation.allocations())),
    _newAllocations(cloneAllocations(newAllocations)) {
  qDeleteAll(newAllocations);
  if (newAllocations.size() > 1) {
    setText(QObject::tr("Split operation into %1 categories").arg(newAllocations.size()));
  } else if (newAllocations.size() == 1) {
    auto category = newAllocations.first()->category();
    auto name = category ? category->name() : "";
    setText(QObject::tr("Set operation category to \"%1\"").arg(name));
  } else {
    setText(QObject::tr("Clear operation split"));
  }
}

SplitOperationCommand::~SplitOperationCommand() {
  qDeleteAll(_oldAllocations);
  qDeleteAll(_newAllocations);
}

QList<Allocation*> SplitOperationCommand::cloneAllocations(const QList<Allocation*>& allocations) {
  QList<Allocation*> result;
  for (auto* allocation : allocations) {
    if (allocation) result.append(new Allocation(allocation->category(), allocation->amount()));
  }
  return result;
}

void SplitOperationCommand::undo() {
  _operation.setAllocations(cloneAllocations(_oldAllocations));
}

void SplitOperationCommand::redo() {
  _operation.setAllocations(cloneAllocations(_newAllocations));
}

SetOperationAmountCommand::SetOperationAmountCommand(Operation& operation,
                                                     double newAmount,
                                                     QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _oldAmount(operation.amount()),
    _newAmount(newAmount) {
  setText(QObject::tr("Set operation amount to %1").arg(newAmount, 0, 'f', 2));
}

void SetOperationAmountCommand::undo() {
  _operation.set_amount(_oldAmount);
}

void SetOperationAmountCommand::redo() {
  _operation.set_amount(_newAmount);
}

SetOperationDateCommand::SetOperationDateCommand(Operation& operation,
                                                 const QDate& newDate,
                                                 QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _oldDate(operation.date()),
    _newDate(newDate) {
  setText(QObject::tr("Set operation date to %1").arg(newDate.toString("dd/MM/yyyy")));
}

void SetOperationDateCommand::undo() {
  _operation.set_date(_oldDate);
  if (_operation.account()) _operation.account()->sortOperations();
}

void SetOperationDateCommand::redo() {
  _operation.set_date(_newDate);
  if (_operation.account()) _operation.account()->sortOperations();
}

SetOperationLabelCommand::SetOperationLabelCommand(Operation& operation,
                                                   const QString& newLabel,
                                                   QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _oldLabel(operation.label()),
    _newLabel(newLabel) {
  setText(QObject::tr("Set operation label"));
}

void SetOperationLabelCommand::undo() {
  _operation.set_label(_oldLabel);
}

void SetOperationLabelCommand::redo() {
  _operation.set_label(_newLabel);
}

SetOperationDetailsCommand::SetOperationDetailsCommand(Operation& operation,
                                                       const QString& newDetails,
                                                       QUndoCommand* parent) :
    QUndoCommand(parent),
    _operation(operation),
    _oldDetails(operation.details()),
    _newDetails(newDetails) {
  setText(QObject::tr("Set operation details"));
}

void SetOperationDetailsCommand::undo() {
  _operation.set_details(_oldDetails);
}

void SetOperationDetailsCommand::redo() {
  _operation.set_details(_newDetails);
}

// SetLeftoverDecisionCommand implementation

SetLeftoverDecisionCommand::SetLeftoverDecisionCommand(Category& category,
                                                       CategoryController* categoryController,
                                                       const QDate& date,
                                                       const LeftoverDecision& newDecision,
                                                       QUndoCommand* parent) :
    QUndoCommand(parent),
    _category(category),
    _categoryController(categoryController),
    _date(date),
    _oldDecision(category.leftoverDecision(date.year(), date.month())),
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
    emit _categoryController->budgetDataChanged();
  }
}

void SetLeftoverDecisionCommand::redo() {
  if (_newDecision.isEmpty()) {
    _category.clearLeftoverDecision(_date.year(), _date.month());
  } else {
    _category.setLeftoverDecision(_date.year(), _date.month(), _newDecision);
  }
  if (_categoryController) {
    emit _categoryController->budgetDataChanged();
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

AddRuleCommand::AddRuleCommand(RuleController* ruleController, Rule* rule,
                               QUndoCommand* parent) :
    QUndoCommand(parent),
    _ruleController(ruleController),
    _rule(rule),
    _ownsRule(true) {
  setText(QObject::tr("Add rule for \"%1\"").arg(rule->labelMatch()));
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
    setText(QObject::tr("Remove rule for \"%1\"").arg(_rule->labelMatch()));
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
      _ruleController->moveRuleDirect(currentIndex, _index);
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

EditRuleCommand::EditRuleCommand(RuleController& ruleController, Rule* rule,
                                 const Category* newCategory,
                                 const QString& newLabelMatch,
                                 double newAmountFilter,
                                 QUndoCommand* parent) :
    QUndoCommand(parent),
    _ruleController(ruleController),
    _rule(rule),
    _oldCategory(rule->category()),
    _newCategory(newCategory),
    _oldLabelMatch(rule->labelMatch()),
    _newLabelMatch(newLabelMatch),
    _oldAmountFilter(rule->amountFilter()),
    _newAmountFilter(newAmountFilter) {
  setText(QObject::tr("Edit rule for \"%1\"").arg(newLabelMatch));
}

void EditRuleCommand::undo() {
  _rule->set_category(_oldCategory);
  _rule->set_labelMatch(_oldLabelMatch);
  _rule->set_amountFilter(_oldAmountFilter);
  _ruleController.ruleModel()->refresh();
  emit _ruleController.rulesChanged();
}

void EditRuleCommand::redo() {
  _rule->set_category(_newCategory);
  _rule->set_labelMatch(_newLabelMatch);
  _rule->set_amountFilter(_newAmountFilter);
  _ruleController.ruleModel()->refresh();
  emit _ruleController.rulesChanged();
}

// MoveRuleCommand implementation
MoveRuleCommand::MoveRuleCommand(RuleController& ruleController, int fromIndex, int toIndex,
                                 QUndoCommand* parent) :
    QUndoCommand(parent),
    _ruleController(ruleController),
    _fromIndex(fromIndex),
    _toIndex(toIndex) {
  setText(QObject::tr("Move rule"));
}

void MoveRuleCommand::undo() {
  _ruleController.moveRuleDirect(_toIndex, _fromIndex);
}

void MoveRuleCommand::redo() {
  _ruleController.moveRuleDirect(_fromIndex, _toIndex);
}
