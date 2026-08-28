#pragma once

#include <QDate>
#include <QList>
#include <QObject>

class Account;
class Allocation;
class BudgetData;
class Operation;
class QUndoStack;

class OperationEditor : public QObject {
  Q_OBJECT

public:
  explicit OperationEditor(BudgetData& budgetData, QUndoStack& undoStack,
                           QObject* parent = nullptr);

  Q_INVOKABLE void add(const QDate& date, double amount, const QString& label,
                       const QString& details, const QList<Allocation*>& allocations);
  Q_INVOKABLE void setBudgetDate(Operation* operation, const QDate& date);
  Q_INVOKABLE void setAmount(Operation* operation, double amount);
  Q_INVOKABLE void setDate(Operation* operation, const QDate& date);
  Q_INVOKABLE void setLabel(Operation* operation, const QString& label);
  Q_INVOKABLE void setDetails(Operation* operation, const QString& details);
  Q_INVOKABLE void setAllocations(Operation* operation, const QList<Allocation*>& allocations);
  Q_INVOKABLE Operation* createCounterpart(Operation* operation, Account* account,
                                           const QString& categoryName);
  Q_INVOKABLE void deleteSelected();

private:
  BudgetData& _budgetData;
  QUndoStack& _undoStack;
};
