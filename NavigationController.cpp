#include <QDate>

#include "Account.h"
#include "BudgetData.h"
#include "CategoryController.h"
#include "NavigationController.h"

NavigationController::NavigationController(BudgetData& budgetData) :
    _budgetDate(QDate::currentDate()),
    _budgetData(budgetData) {
}

Account* NavigationController::currentAccount() const {
  return _currentAccount;
}

void NavigationController::set_currentAccount(Account* account) {
  if (account) {
    int index = _budgetData.accountIndex(account);
    if (index != _currentAccountIndex) {
      _currentAccount = account;
      set_currentAccountIndex(index);
    }
  }
}

int NavigationController::currentAccountIndex() const {
  return _currentAccountIndex;
}

void NavigationController::set_currentAccountIndex(int index) {
  int accountCount = _budgetData.accountCount();
  if (index >= -1 && index < accountCount) {
    bool indexChanged = (index != _currentAccountIndex);
    _currentAccountIndex = index;

    Account* newAccount = _budgetData.accountAt(index);
    bool accountChanged = (_currentAccount != newAccount);
    _currentAccount = newAccount;

    // Always update the operation model with the current account pointer
    // (the model has its own guard to avoid unnecessary resets)
    _budgetData.operationModel()->setAccount(newAccount);

    // Emit signals if either the index or the actual account pointer changed
    // (account pointer can change even with same index when loading a new file)
    if (indexChanged) {
      emit currentAccountIndexChanged();
    }
    if (accountChanged) {
      emit currentAccountChanged();
    }
  }
}

void NavigationController::previousMonth() {
  QDate date = _budgetDate.addMonths(-1);
  set_budgetDate(QDate(date.year(), date.month(), 1));
}

void NavigationController::nextMonth() {
  QDate date = _budgetDate.addMonths(1);
  set_budgetDate(QDate(date.year(), date.month(), 1));
}

void NavigationController::previousOperation(bool extendSelection) {
  Account* account = currentAccount();
  if (!account) return;

  int currentIndex = account->currentOperationIndex();
  if (currentIndex > 0) {
    int newIndex = currentIndex - 1;
    account->selectAt(newIndex, extendSelection);
    account->set_currentOperationIndex(newIndex);
  }
}

void NavigationController::nextOperation(bool extendSelection) {
  Account* account = currentAccount();
  if (!account) return;

  int currentIndex = account->currentOperationIndex();
  if (currentIndex < account->operationCount() - 1) {
    int newIndex = currentIndex + 1;
    account->selectAt(newIndex, extendSelection);
    account->set_currentOperationIndex(newIndex);
  }
}

void NavigationController::showOperationsTab() {
  set_currentTabIndex(0);
}

void NavigationController::showBudgetTab() {
  set_currentTabIndex(1);
}

void NavigationController::navigateToOperation(Operation* operation) {
  int accountIndex = _budgetData.accountIndex(operation->account());

  if (accountIndex < 0) {
    return;
  }

  // Switch to the account
  set_currentAccountIndex(accountIndex);

  // Find the operation in the account
  Account* account = operation->account();
  account->set_currentOperation(operation);
  account->select(operation, false);
  int operationIndex = account->operationIndex(operation);

  // Emit signal so OperationList can focus the operation
  emit operationSelected(operationIndex);

  // Switch to Operations tab
  set_currentTabIndex(0);
}

void NavigationController::onNavigationStateLoaded(int tabIndex, const QDate& budgetDate,
                                                   int accountIndex, int categoryIndex, int operationIndex) {
  set_currentTabIndex(tabIndex);
  set_budgetDate(budgetDate);

  set_currentAccountIndex(accountIndex);
  set_currentCategoryIndex(categoryIndex);

  // Set the operation index on the current account
  Account* account = currentAccount();
  if (account) {
    account->set_currentOperationIndex(operationIndex);
  }
}
