#pragma once

#include <QDate>
#include <QObject>

#include "PropertyMacros.h"

class Account;
class BudgetData;
class CategoryController;
class Operation;

class NavigationController : public QObject {
  Q_OBJECT

  // Tab navigation
  PROPERTY_RW(int, currentTabIndex, 0)

  PROPERTY_RW(QDate, budgetDate, QDate::currentDate())

  // Category navigation
  PROPERTY_RW(int, currentCategoryIndex, -1)

  // Account navigation (custom setter to update operation model)
  PROPERTY_RW_CUSTOM(int, currentAccountIndex, -1)

  // Current account (computed from currentAccountIndex)
  Q_PROPERTY(Account* currentAccount READ currentAccount NOTIFY currentAccountChanged)

public:
  explicit NavigationController(BudgetData& budgetData);

  // Current account getter
  Account* currentAccount() const;

  // Month navigation
  Q_INVOKABLE void previousMonth();
  Q_INVOKABLE void nextMonth();

  // Operation navigation
  Q_INVOKABLE void previousOperation(bool extendSelection = false);
  Q_INVOKABLE void nextOperation(bool extendSelection = false);

  // Tab shortcuts
  Q_INVOKABLE void showOperationsTab();
  Q_INVOKABLE void showBudgetTab();

  // Cross-navigation (switch account and select operation)
  Q_INVOKABLE void navigateToOperation(const QString& accountName, const QDate& date,
                                       const QString& description, double amount);

public slots:
  // Called when FileController loads navigation state from a file
  void onNavigationStateLoaded(int tabIndex, const QDate& budgetDate,
                               int accountIndex, int categoryIndex, int operationIndex);

signals:
  void operationSelected(int index);  // Emitted when an operation is selected via navigateToOperation()
  void currentAccountChanged();       // Emitted when current account changes

private:
  BudgetData& _budgetData;
  Account* _currentAccount = nullptr;  // Cached pointer to current account
};
