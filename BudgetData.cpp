#include "BudgetData.h"
#include <QDate>
#include <QDebug>

#include "AccountListModel.h"
#include "CategoryController.h"
#include "NavigationController.h"
#include "OperationListModel.h"
#include "UndoCommands.h"

void BudgetData::setNavigationController(NavigationController* navController) {
  _navController = navController;
  if (_navController) {
    connect(_navController, &NavigationController::currentAccountIndexChanged,
            this, &BudgetData::currentAccountChanged);
  }
}

void BudgetData::setCategoryController(CategoryController* categoryController) {
  _categoryController = categoryController;
}

BudgetData::BudgetData(QUndoStack& undoStack) :
    _undoStack(undoStack),
    _operationModel(new OperationListModel(this)),
    _accountModel(new AccountListModel(_accounts, this)) {
  connect(_operationModel, &OperationListModel::operationDataChanged,
          this, &BudgetData::operationDataChanged);
}

BudgetData::~BudgetData() {
  clear();
}

int BudgetData::accountCount() const {
  return _accounts.size();
}

QList<Account*> BudgetData::accounts() const {
  return _accounts;
}

Account* BudgetData::accountAt(int index) const {
  if (index >= 0 && index < _accounts.size()) {
    return _accounts[index];
  }
  return nullptr;
}

Account* BudgetData::currentAccount() const {
  if (_navController) {
    return accountAt(_navController->currentAccountIndex());
  }
  return nullptr;
}

Account* BudgetData::accountByName(const QString& name) const {
  for (Account* account : _accounts) {
    if (account->name() == name) {
      return account;
    }
  }
  return nullptr;
}

int BudgetData::accountIndex(Account* account) const {
  return _accounts.indexOf(account);
}

void BudgetData::renameCurrentAccount(const QString& newName) {
  if (!_navController) return;
  Account* account = accountAt(_navController->currentAccountIndex());
  if (account && !newName.isEmpty() && account->name() != newName) {
    _undoStack.push(new RenameAccountCommand(*account, _accountModel,
                                             account->name(), newName));
  }
}

void BudgetData::addAccount(Account* account) {
  if (account) {
    account->setParent(this);
    _accounts.append(account);
    emit accountCountChanged();
  }
}

void BudgetData::removeAccount(int index) {
  if (index >= 0 && index < _accounts.size()) {
    delete _accounts.takeAt(index);
    emit accountCountChanged();
  }
}

Account* BudgetData::takeAccount(Account* account) {
  int index = _accounts.indexOf(account);
  if (index >= 0) {
    // If this is the current account, update navigation before removing
    if (_navController && _navController->currentAccountIndex() == index) {
      // Select previous account, or -1 if this was the only account
      int newIndex = _accounts.size() > 1 ? qMax(0, index - 1) : -1;
      _navController->set_currentAccountIndex(newIndex);
    } else if (_navController && _navController->currentAccountIndex() > index) {
      // Adjust index if removing an account before the current one
      _navController->set_currentAccountIndex(_navController->currentAccountIndex() - 1);
    }

    Account* acc = _accounts.takeAt(index);
    acc->setParent(nullptr);  // Release Qt ownership
    _accountModel->refresh();
    emit accountCountChanged();
    return acc;
  }
  return nullptr;
}

void BudgetData::selectAccount(int index) {
  if (_navController && index >= 0 && index < _accounts.size()) {
    _navController->set_currentAccountIndex(index);
  }
}

void BudgetData::clearAccounts() {
  _operationModel->setAccount(nullptr);
  qDeleteAll(_accounts);
  _accounts.clear();
  _accountModel->refresh();
  emit accountCountChanged();
}

void BudgetData::addOperation(const QDate& date, double amount, const QString& label, const QString& details, const QVariantList& allocations) {
  if (!_operationModel) return;
  if (!_navController) return;
  Account* account = accountAt(_navController->currentAccountIndex());
  if (!account) return;

  QList<Allocation*> allocationList;
  for (auto alloc : allocations) {
    QVariantMap m = alloc.toMap();
    allocationList.append(new Allocation(_categoryController->getCategoryByName(m["category"].toString()),
                                         m["amount"].toDouble()));
  }

  auto operation = new Operation(account, date, amount, label, details, allocationList);
  _undoStack.push(new AddOperationCommand(operation, *account, *_operationModel));
}

void BudgetData::setOperationBudgetDate(Operation* operation, const QDate& newBudgetDate) {
  if (!operation) return;

  QDate oldBudgetDate = operation->budgetDate();
  if (oldBudgetDate != newBudgetDate) {
    _undoStack.push(new SetOperationBudgetDateCommand(*operation, _operationModel,
                                                      oldBudgetDate, newBudgetDate));
  }
}

void BudgetData::setOperationAmount(Operation* operation, double newAmount) {
  if (!operation) return;

  double oldAmount = operation->amount();
  if (!qFuzzyCompare(oldAmount, newAmount)) {
    _undoStack.push(new SetOperationAmountCommand(*operation, _operationModel,
                                                  oldAmount, newAmount));
  }
}

void BudgetData::setOperationDate(Operation* operation, const QDate& newDate) {
  if (!operation) return;

  QDate oldDate = operation->date();
  if (oldDate != newDate) {
    _undoStack.push(new SetOperationDateCommand(*operation, _operationModel,
                                                oldDate, newDate));
  }
}

void BudgetData::setOperationLabel(Operation* operation, const QString& newLabel) {
  if (!operation) return;

  QString oldLabel = operation->label();
  if (oldLabel != newLabel) {
    _undoStack.push(new SetOperationLabelCommand(*operation, _operationModel,
                                                 oldLabel, newLabel));
  }
}

void BudgetData::setOperationDetails(Operation* operation, const QString& newDetails) {
  if (!operation) return;

  QString oldDetails = operation->details();
  if (oldDetails != newDetails) {
    _undoStack.push(new SetOperationDetailsCommand(*operation, _operationModel,
                                                   oldDetails, newDetails));
  }
}

void BudgetData::setOperationAllocations(Operation* operation, const QVariantList& allocations) {
  if (!operation) return;

  // Convert QVariantList to QList<Allocation>
  QList<Allocation*> newAllocations;
  for (const QVariant& v : allocations) {
    QVariantMap m = v.toMap();
    newAllocations.append(new Allocation(
        _categoryController->getCategoryByName(m["category"].toString()),
        m["amount"].toDouble()));
  }

  // Only create command if something changed
  if (!operation->sameAllocations(newAllocations)) {
    _undoStack.push(new SplitOperationCommand(*operation, _operationModel,
                                              newAllocations));
  } else {
    qDeleteAll(newAllocations);
  }
}

Operation* BudgetData::createCounterPart(Operation* operation, Account* targetAccount) {
  if (!operation || !targetAccount) return nullptr;

  QList<Allocation*> newAllocations;
  for (auto allocation : operation->allocations()) {
    newAllocations.append(new Allocation(allocation->category(), -allocation->amount()));
  }

  auto newOperation = new Operation(targetAccount, operation->date(), -operation->amount(),
                                    operation->label(), operation->details(), newAllocations);

  _undoStack.push(new AddOperationCommand(newOperation, *targetAccount, *_operationModel));
  return newOperation;
}

void BudgetData::deleteSelectedOperations() {
  if (!_operationModel) return;
  if (!_navController) return;
  Account* account = accountAt(_navController->currentAccountIndex());
  if (!account) return;

  QUndoCommand* macroCommand = new QUndoCommand();

  for (Operation* op : account->selectedOperations()) {
    new DeleteOperationCommand(op, *account, *_operationModel, macroCommand);
  }
  _undoStack.push(macroCommand);
}

void BudgetData::deleteCategory(Category* category) {
  if (!category || !_categoryController) return;
  if (!_operationModel) return;

  QUndoCommand* macroCommand = new QUndoCommand();

  // Update all operations that reference this category to remove it from their allocations
  for (Account* account : _accounts) {
    for (Operation* op : account->operations()) {
      // Create a new allocations list without the deleted category
      QList<Allocation*> newAllocations;
      for (auto alloc : op->allocations()) {
        if (alloc->category() != category) {
          newAllocations.append(new Allocation(alloc->category(), alloc->amount()));
        }
      }
      // Only create a command if the allocations actually changed
      if (newAllocations.size() != op->allocations().size()) {
        new SplitOperationCommand(*op, _operationModel,
                                  newAllocations, macroCommand);
      }
    }
  }

  _undoStack.push(new DeleteCategoryCommand(_categoryController, category));
  _undoStack.push(macroCommand);
}

void BudgetData::clear() {
  clearAccounts();
  _undoStack.clear();
  _undoStack.setClean();
}

void BudgetData::undo() {
  _undoStack.undo();
}

void BudgetData::redo() {
  _undoStack.redo();
}
