#pragma once

#include <QtQml/qqml.h>
#include <QAbstractListModel>

#include "PropertyMacros.h"

class CategoryController;

class LeftoverListModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT

  PROPERTY_RW(QDate, date, QDate::currentDate())

  Q_PROPERTY(double totalToSave READ totalToSave NOTIFY totalsChanged)
  Q_PROPERTY(double totalToReport READ totalToReport NOTIFY totalsChanged)
  Q_PROPERTY(double totalFromReport READ totalFromReport NOTIFY totalsChanged)
  Q_PROPERTY(double netReport READ netReport NOTIFY totalsChanged)

public:
  enum Roles {
    NameRole = Qt::UserRole + 1,
    BudgetLimitRole,
    SpentRole,
    AccumulatedRole,
    LeftoverRole,
    IsIncomeRole,
    SaveAmountRole,
    ReportAmountRole,
    IsBalancedRole
  };
  Q_ENUM(Roles)

  explicit LeftoverListModel(CategoryController& categories);

  // QAbstractListModel interface
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  // Totals properties
  double totalToSave() const { return _totalToSave; }
  double totalToReport() const { return _totalToReport; }
  double totalFromReport() const { return _totalFromReport; }
  double netReport() const { return _netReport; }

  // Refresh data
  Q_INVOKABLE void refresh();

  // Getters for dynamic properties (called from QML when data changes)
  Q_INVOKABLE double getSaveAmount(int row) const;
  Q_INVOKABLE double getReportAmount(int row) const;
  Q_INVOKABLE bool getIsBalanced(int row) const;

signals:
  void yearChanged();
  void monthChanged();
  void totalsChanged();

private:
  struct LeftoverItem {
    QString name;
    double budgetLimit = 0.0;
    double spent = 0.0;
    double accumulated = 0.0;
    double leftover = 0.0;
    bool isIncome = false;
    double saveAmount = 0.0;
    double reportAmount = 0.0;
  };

  CategoryController& _categories;
  QVector<LeftoverItem> _items;

  // Cached totals
  double _totalToSave = 0.0;
  double _totalToReport = 0.0;
  double _totalFromReport = 0.0;
  double _netReport = 0.0;

  void refreshTotals();
  void onLeftoverDataChanged();
};
