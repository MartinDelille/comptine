#pragma once

#include <QObject>

class Category;
class RuleController;
class BudgetData;
class QUndoStack;

class RuleEditor : public QObject {
  Q_OBJECT

public:
  explicit RuleEditor(RuleController& controller, BudgetData& budgetData, QUndoStack& undoStack,
                      QObject* parent = nullptr);

  Q_INVOKABLE void add(const Category* category, const QString& labelMatch, double amountFilter = 0);
  Q_INVOKABLE void remove(int index);
  Q_INVOKABLE void edit(int index, const Category* category, const QString& labelMatch,
                        double amountFilter = 0);
  Q_INVOKABLE void move(int fromIndex, int toIndex);
  Q_INVOKABLE int applyToUncategorized(const Category* category, const QString& labelMatch,
                                       double amountFilter = 0);

private:
  RuleController& _controller;
  BudgetData& _budgetData;
  QUndoStack& _undoStack;
};
