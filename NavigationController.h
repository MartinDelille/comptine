#pragma once

#include <QDate>
#include <QObject>
#include <QQmlEngine>

#include "PropertyMacros.h"

class BudgetData;
class Operation;

class NavigationController : public QObject {
  Q_OBJECT
  QML_ELEMENT

  // Tab navigation
  PROPERTY_RW(int, currentTabIndex, 0)

  PROPERTY_RW(QDate, budgetDate, QDate::currentDate())

public:
  explicit NavigationController(BudgetData& budgetData);

  // Month navigation
  Q_INVOKABLE void previousMonth();
  Q_INVOKABLE void nextMonth();

  // Cross-navigation (switch account and select operation)
  Q_INVOKABLE void navigateToOperation(Operation* operation);

private:
  BudgetData& _budgetData;
};
