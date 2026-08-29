#pragma once

#include <QObject>
#include <QQmlEngine>

class BudgetData;
class QUndoStack;

class AccountEditor : public QObject {
  Q_OBJECT

public:
  explicit AccountEditor(BudgetData& budgetData, QUndoStack& undoStack,
                         QObject* parent = nullptr);

  Q_INVOKABLE void renameCurrentAccount(const QString& name);

private:
  BudgetData& _budgetData;
  QUndoStack& _undoStack;
};
