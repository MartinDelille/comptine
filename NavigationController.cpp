#include <QDate>

#include "Account.h"
#include "BudgetData.h"
#include "NavigationController.h"
#include "Operation.h"

NavigationController::NavigationController(BudgetData& budgetData) :
    _budgetDate(QDate::currentDate()),
    _budgetData(budgetData) {
}

void NavigationController::previousMonth() {
  QDate date = _budgetDate.addMonths(-1);
  set_budgetDate(QDate(date.year(), date.month(), 1));
}

void NavigationController::nextMonth() {
  QDate date = _budgetDate.addMonths(1);
  set_budgetDate(QDate(date.year(), date.month(), 1));
}

void NavigationController::navigateToOperation(Operation* operation) {
  Account* account = operation->account();
  // Switch to the account
  _budgetData.set_currentAccount(account);

  // Find the operation in the account
  account->select(operation, false);

  set_currentTabIndex(0);
}
