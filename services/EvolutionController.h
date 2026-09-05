#pragma once

#include <qqmlintegration.h>
#include <QAbstractTableModel>
#include <QDate>

#include "utils/PropertyMacros.h"

class BudgetData;
class Category;
class CategoryController;

class EvolutionController : public QAbstractTableModel {
  Q_OBJECT

  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
  Q_PROPERTY(int monthCount READ monthCount NOTIFY availableMonthsChanged)
  PROPERTY_RO(int, currentMonthIndex)
  PROPERTY_RO(QDate, firstMonth)
  PROPERTY_RO(QDate, lastMonth)

public:
  enum Roles {
    DisplayRole = Qt::DisplayRole,
    CategoryNameRole = Qt::UserRole + 1,
    MonthDateRole,
    BudgetRole,
    SpentRole,
    LeftoverRole,
    SavedRole,
    ReportedRole,
    AccumulatedRole,
    CurrentMonthRole,
    CurrentCategoryRole,
  };
  Q_ENUM(Roles)

  EvolutionController(BudgetData& budgetData, CategoryController& categories,
                      QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  int monthCount() const;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  Q_INVOKABLE QVariant headerData(int section, Qt::Orientation orientation,
                                  int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

signals:
  void countChanged();
  void availableMonthsChanged();

private slots:
  void resetModel();
  void refreshData();
  void refreshSelection();

private:
  QDate monthDate(int column) const;
  QList<QDate> calculateAvailableMonths() const;
  void updateAvailableMonths();
  QDate historyStart() const;
  QDate historyEnd() const;

  BudgetData& _budgetData;
  CategoryController& _categories;
  QList<QDate> _availableMonths;
};
