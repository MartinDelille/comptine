#pragma once

#include <QDate>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QVariantList>

class Account;
class Allocation;
class Category;
class BudgetData;
class Operation;
class QUndoStack;

class OperationEditor : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool editing READ isEditing NOTIFY editingChanged)
  Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoStateChanged)
  Q_PROPERTY(bool canRedo READ canRedo NOTIFY undoStateChanged)

public:
  explicit OperationEditor(BudgetData& budgetData, QUndoStack& undoStack,
                           QObject* parent = nullptr);

  bool isEditing() const { return _editingOperation != nullptr; }
  bool canUndo() const;
  bool canRedo() const;
  Q_INVOKABLE bool beginEditing(Operation* operation);
  Q_INVOKABLE Operation* beginNew(const QDate& date, double amount,
                                  const QString& label, const QString& details);
  Q_INVOKABLE void endEditing(bool commit);
  Q_INVOKABLE void undo();
  Q_INVOKABLE void redo();

  Q_INVOKABLE void add(const QDate& date, double amount, const QString& label,
                       const QString& details, const QVariantList& allocations);
  Q_INVOKABLE void setBudgetDate(Operation* operation, const QDate& date);
  Q_INVOKABLE void setAmount(Operation* operation, double amount);
  Q_INVOKABLE void setDate(Operation* operation, const QDate& date);
  Q_INVOKABLE void setLabel(Operation* operation, const QString& label);
  Q_INVOKABLE void setDetails(Operation* operation, const QString& details);
  Q_INVOKABLE void addAllocation(Operation* operation, const Category* category, double amount);
  Q_INVOKABLE void removeAllocation(Operation* operation, int index);
  Q_INVOKABLE void setAllocationCategory(Operation* operation, int index, const Category* category);
  Q_INVOKABLE void setAllocationAmount(Operation* operation, int index, double amount);
  Q_INVOKABLE void normalizeAllocations(Operation* operation);
  Q_INVOKABLE Operation* createCounterpart(Operation* operation, Account* account,
                                           const QString& categoryName);
  Q_INVOKABLE void deleteSelected();

signals:
  void editingChanged();
  void editingStarted(Operation* operation);
  void editingFinished();
  void editingCancelled();
  void transactionRejected();
  void undoRejected();
  void redoRejected();
  void undoStateChanged();

private:
  void setAllocationList(Operation* operation, const QList<Allocation*>& allocations);

  BudgetData& _budgetData;
  QUndoStack& _undoStack;
  QPointer<Operation> _editingOperation;
  // Number of undo commands before beginMacro(), used to detect no-op edits.
  int _editingStartCount = -1;
};
