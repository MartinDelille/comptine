#pragma once

#include <qqmlintegration.h>
#include <QAbstractTableModel>
#include <QDate>
#include <QStringList>

#include "utils/PropertyMacros.h"

class BudgetData;
class Category;
class CategoryController;

class EvolutionController : public QAbstractTableModel {
  Q_OBJECT

  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
  Q_PROPERTY(int monthCount READ monthCount NOTIFY availableMonthsChanged)
  Q_PROPERTY(QStringList availableMonthLabels READ availableMonthLabels NOTIFY availableMonthsChanged)
  Q_PROPERTY(int summaryStartIndex READ summaryStartIndex NOTIFY summaryStartMonthChanged)
  Q_PROPERTY(int summaryEndIndex READ summaryEndIndex NOTIFY summaryEndMonthChanged)
  PROPERTY_RW(int, selectedMetric, 0)
  PROPERTY_RW_CUSTOM(QDate, summaryStartMonth, QDate())
  PROPERTY_RW_CUSTOM(QDate, summaryEndMonth, QDate())
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
    SpentAverageRole,
    ReportedSumRole,
    MonthlySumRole,
    CurrentMonthRole,
    CurrentCategoryRole,
    BudgetLimitChangeRole,
  };
  Q_ENUM(Roles)

  EvolutionController(BudgetData& budgetData, CategoryController& categories,
                      QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  int monthCount() const;
  QStringList availableMonthLabels() const;
  int summaryStartIndex() const;
  int summaryEndIndex() const;
  Q_INVOKABLE void setSummaryStartMonthIndex(int index);
  Q_INVOKABLE void setSummaryEndMonthIndex(int index);
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  Q_INVOKABLE QVariant headerData(int section, Qt::Orientation orientation,
                                  int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

signals:
  void countChanged();
  void availableMonthsChanged();

private slots:
  void refreshCategoryStructure();
  void refreshData();
  void refreshMonthSelection();
  void refreshSelection();
  void refreshSummary();

private:
  QDate monthDate(int column) const;
  double metricValue(const Category* category, const QDate& month,
                     int metric) const;
  double categorySum(const Category* category, int metric,
                     const QDate& start, const QDate& end) const;
  double monthlySum(const QDate& month) const;
  QDate clampMonth(const QDate& month) const;
  int summaryMonthCount() const;
  QList<QDate> calculateAvailableMonths() const;
  bool updateAvailableMonths();
  void resetModel();
  QDate historyStart() const;
  QDate historyEnd() const;

  BudgetData& _budgetData;
  CategoryController& _categories;
  QList<QDate> _availableMonths;
  // Keeps the default full-history range expandable until the user edits it.
  bool _summaryRangeCustomized = false;
};
