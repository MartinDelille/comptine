#pragma once

#include <QDate>
#include <QObject>

class Allocation;
class Category;
class CategoryController;
class BudgetData;
class QUndoStack;

class CategoryEditor : public QObject {
  Q_OBJECT

public:
  explicit CategoryEditor(CategoryController& controller, BudgetData& budgetData,
                          QUndoStack& undoStack, QObject* parent = nullptr);

  Q_INVOKABLE Allocation* createAllocation(const QString& categoryName, double amount);
  Q_INVOKABLE Category* edit(const QString& name, double budgetLimit,
                             Category* category = nullptr, QDate budgetDate = {});
  Q_INVOKABLE void remove(Category* category);
  Q_INVOKABLE void setSaveAmount(Category* category, const QDate& date, double amount);
  Q_INVOKABLE void setReportAmount(Category* category, const QDate& date, double amount);

private:
  CategoryController& _controller;
  BudgetData& _budgetData;
  QUndoStack& _undoStack;
};
