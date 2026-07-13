#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

#include "Operation.h"
#include "PropertyMacros.h"

class Account : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
  PROPERTY_RW(QString, name, QString())

  // Filenames (base name only) of CSV files previously imported into this account
  Q_PROPERTY(QStringList importSourcePrefixes READ importSourcePrefixes NOTIFY importSourcePrefixesChanged)

  PROPERTY_RO(Operation*, currentOperation)
  Q_PROPERTY(int currentOperationIndex READ currentOperationIndex WRITE set_currentOperationIndex
                 NOTIFY currentOperationChanged)

  // Selection properties (pointer-based, survives sorting)
  Q_PROPERTY(int selectionCount READ selectionCount NOTIFY selectionChanged)
  Q_PROPERTY(double selectedTotal READ selectedTotal NOTIFY selectionChanged)

public:
  enum Roles {
    DateRole = Qt::UserRole + 1,
    AmountRole,
    LabelRole,
    BalanceRole,
    SelectedRole,
    OperationRole,
  };
  Q_ENUM(Roles)

  explicit Account(const QString& name);

  // QAbstractListModel interface
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role) override;
  QHash<int, QByteArray> roleNames() const override;

  void refresh();  // Recalculate balances and emit dataChanged for all rows

  // Import source management
  QStringList importSourcePrefixes() const;
  void addImportSourcePrefix(const QString& filename);
  void setImportSourcePrefixes(const QStringList& sources);

  // Current operation index (computed from currentOperation pointer)
  int currentOperationIndex() const;
  void set_currentOperationIndex(int index);

  QList<Operation*> operations() const;

  // Operation navigation
  Q_INVOKABLE void previousOperation(bool extendSelection = false);
  Q_INVOKABLE void nextOperation(bool extendSelection = false);

  void addOperation(Operation* operation, bool sort = true);
  bool removeOperation(Operation* operation);  // Remove by pointer, returns true if found
  void clearOperations();
  void sortOperations();  // Re-sort operations by date (most recent first)
  bool hasOperation(const QDate& date, double amount, const QString& label) const;

  Operation* operationAt(int index) const;
  int operationIndex(Operation* operation) const;

  // Selection management (Excel-like behavior)
  // Uses currentOperation as anchor for range selection
  bool isSelected(Operation* operation) const;
  Q_INVOKABLE bool isSelectedAt(int index) const;
  void select(Operation* operation, bool extend = false);
  Q_INVOKABLE void selectAt(int index, bool extend = false);
  void toggleSelection(Operation* operation);
  Q_INVOKABLE void toggleSelectionAt(int index);
  Q_INVOKABLE void selectRange(int fromIndex, int toIndex);
  Q_INVOKABLE void selectAll();
  Q_INVOKABLE void clearSelection();
  int selectionCount() const;
  double selectedTotal() const;
  QSet<Operation*> selectedOperations() const;
  QString selectedOperationsAsCsv() const;

  Q_INVOKABLE double balanceAt(int index) const;

signals:
  void countChanged();
  void selectionChanged();
  void importSourcePrefixesChanged();

private:
  void recalculateBalances();

  Operation* _currentOperation = nullptr;
  QList<Operation*> _operations;
  QSet<Operation*> _selectedOperations;
  QStringList _importSources;
  QVector<double> _balances;
};
