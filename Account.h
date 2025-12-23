#pragma once

#include <QList>
#include <QObject>
#include <QSet>
#include <QString>

#include "Operation.h"
#include "PropertyMacros.h"

class Account : public QObject {
  Q_OBJECT
  QML_ELEMENT
  PROPERTY_RW(QString, name, QString())
  PROPERTY_RO(int, operationCount)
  PROPERTY_RW(Operation*, currentOperation, nullptr)

  // Computed property: index of currentOperation in operations list
  // Uses currentOperationChanged signal since it changes when currentOperation changes
  Q_PROPERTY(int currentOperationIndex READ currentOperationIndex WRITE set_currentOperationIndex
                 NOTIFY currentOperationChanged)

  // Selection properties (pointer-based, survives sorting)
  Q_PROPERTY(int selectionCount READ selectionCount NOTIFY selectionChanged)
  Q_PROPERTY(double selectedTotal READ selectedTotal NOTIFY selectionChanged)

public:
  explicit Account(QObject* parent = nullptr);
  explicit Account(const QString& name, QObject* parent = nullptr);

  // Current operation index (computed from currentOperation pointer)
  int currentOperationIndex() const;
  void set_currentOperationIndex(int index);

  QList<Operation*> operations() const;

  void addOperation(Operation* operation);
  void appendOperation(Operation* operation);  // Append without sorting (for file loading)
  void removeOperation(int index);
  bool removeOperation(Operation* operation);  // Remove by pointer, returns true if found
  void clearOperations();
  void sortOperations();  // Re-sort operations by date (most recent first)
  bool hasOperation(const QDate& date, double amount, const QString& label) const;

  Q_INVOKABLE Operation* getOperation(int index) const;

  // Selection management (Excel-like behavior)
  // Uses currentOperation as anchor for range selection
  bool isSelected(Operation* operation) const;
  Q_INVOKABLE bool isSelectedAt(int index) const;
  void select(Operation* operation, bool extend = false);
  Q_INVOKABLE void selectAt(int index, bool extend = false);
  void toggleSelection(Operation* operation);
  Q_INVOKABLE void toggleSelectionAt(int index);
  Q_INVOKABLE void selectRange(int fromIndex, int toIndex);
  Q_INVOKABLE void clearSelection();
  int selectionCount() const;
  double selectedTotal() const;
  QSet<Operation*> selectedOperations() const;
  QString selectedOperationsAsCsv() const;

signals:
  void selectionChanged();

private:
  QList<Operation*> _operations;
  QSet<Operation*> _selectedOperations;
};
