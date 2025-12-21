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

Account* BudgetData::getAccount(int index) const {
  if (index >= 0 && index < _accounts.size()) {
    return _accounts[index];
  }
  return nullptr;
}

Account* BudgetData::currentAccount() const {
  if (_navController) {
    return getAccount(_navController->currentAccountIndex());
  }
  return nullptr;
}

Account* BudgetData::getAccountByName(const QString& name) const {
  for (Account* account : _accounts) {
    if (account->name() == name) {
      return account;
    }
  }
  return nullptr;
}

void BudgetData::renameCurrentAccount(const QString& newName) {
  if (!_navController) return;
  Account* account = getAccount(_navController->currentAccountIndex());
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

void BudgetData::addOperation(const QDate& date, double amount, const QString& description, const QVariantList& allocations) {
  if (!_operationModel) return;
  if (!_navController) return;
  Account* account = getAccount(_navController->currentAccountIndex());
  if (!account) return;

  Category* category = nullptr;
  QList<CategoryAllocation> allocationList;
  if (allocations.size() == 1) {
    QVariantMap m = allocations.first().toMap();
    category = _categoryController->getCategoryByName(m["category"].toString());
  } else {
    for (auto alloc : allocations) {
      QVariantMap m = alloc.toMap();
      allocationList.append(CategoryAllocation{
          _categoryController->getCategoryByName(m["category"].toString()),
          m["amount"].toDouble(),

      });
    }
  }

  auto operation = new Operation(date, amount, category, description, allocationList);
  _undoStack.push(new AddOperationCommand(operation, *account, *_operationModel));
}

void BudgetData::setOperationCategory(Operation* operation, const Category* newCategory) {
  if (!operation) return;

  auto oldCategory = operation->category();
  if (oldCategory != newCategory) {
    _undoStack.push(new SetOperationCategoryCommand(*operation, _operationModel,
                                                    oldCategory, newCategory));
  }
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

void BudgetData::setOperationDescription(Operation* operation, const QString& newDescription) {
  if (!operation) return;

  QString oldDescription = operation->description();
  if (oldDescription != newDescription) {
    _undoStack.push(new SetOperationDescriptionCommand(*operation, _operationModel,
                                                       oldDescription, newDescription));
  }
}

void BudgetData::splitOperation(Operation* operation, const QVariantList& allocations) {
  if (!operation) return;

  // Convert QVariantList to QList<CategoryAllocation>
  QList<CategoryAllocation> newAllocations;
  for (const QVariant& v : allocations) {
    QVariantMap m = v.toMap();
    CategoryAllocation alloc;
    alloc.category = _categoryController->getCategoryByName(m["category"].toString());
    alloc.amount = m["amount"].toDouble();
    newAllocations.append(alloc);
  }

  // Get current state
  auto oldCategory = operation->category();
  QList<CategoryAllocation> oldAllocations = operation->allocationsList();

  // Only create command if something changed
  if (newAllocations != oldAllocations || (newAllocations.size() == 1 && newAllocations.first().category != oldCategory)) {
    _undoStack.push(new SplitOperationCommand(*operation, _operationModel,
                                              oldCategory, oldAllocations, newAllocations));
  }
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
