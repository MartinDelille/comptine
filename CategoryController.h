#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUndoStack>
#include <QVariant>

#include "Category.h"
#include "NavigationController.h"
#include "PropertyMacros.h"

class Account;
class BudgetData;

class CategoryController : public QAbstractListModel {
  Q_OBJECT

  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
  Q_PROPERTY(double totalToSave READ totalToSave NOTIFY leftoverDataChanged)
  Q_PROPERTY(double totalToReport READ totalToReport NOTIFY leftoverDataChanged)
  Q_PROPERTY(double totalFromReport READ totalFromReport NOTIFY leftoverDataChanged)
  Q_PROPERTY(double netReport READ netReport NOTIFY leftoverDataChanged)
  PROPERTY_RO(Category*, current)

public:
  enum Roles {
    CategoryRole = Qt::UserRole + 1,
    AmountRole,
    AccumulatedRole,
    LeftoverRole,
    SaveAmountRole,
    ReportAmountRole,
  };
  Q_ENUM(Roles)

  explicit CategoryController(BudgetData& budgetData,
                              const NavigationController& navigation,
                              QUndoStack& undoStack);

  // QAbstractListModel interface
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  // Leftover totals accessors
  double totalToSave() const;
  double totalToReport() const;
  double totalFromReport() const;
  double netReport() const;

  // Category accessors
  QList<Category*> categories() const;
  Q_INVOKABLE Category* at(int index) const;
  Q_INVOKABLE Category* getCategoryByName(const QString& name) const;
  Q_INVOKABLE QStringList categoryNames() const;

  // Category management
  Q_INVOKABLE Category* addCategory(const QString& name, double budgetLimit);
  void addCategory(Category* category);
  void removeCategory(int index);
  void clear();
  Category* takeCategoryByName(const QString& name);  // Remove without deleting

  // Category editing (undoable)
  Q_INVOKABLE void editCategory(const QString& originalName, const QString& newName, double newBudgetLimit);

  // Budget calculations (aggregates across all accounts)
  Q_INVOKABLE double spentInCategory(const Category* categoryName, const QDate& budgetDate) const;
  Q_INVOKABLE QVariantList operationsForCategory(const Category* category, const QDate& date) const;

  // Leftover calculations
  // Calculate the leftover for a category in a month: budget - spent (+ accumulated from previous months)
  Q_INVOKABLE double leftoverForCategory(const Category* category, const QDate& date) const;

  // Get accumulated leftover from previous months (sum of all "report" decisions before this month)
  Q_INVOKABLE double accumulatedLeftover(const QString& categoryName, const QDate& date) const;

  // Get the current leftover decision for a category in a month
  Q_INVOKABLE QVariantMap leftoverDecision(const QString& categoryName, const QDate& date) const;

  // Get summary of all categories with leftover info for a month (for the leftover dialog)
  Q_INVOKABLE QVariantList leftoverSummary(const QDate& date) const;

  // Get monthly totals for savings and leftover transfers
  Q_INVOKABLE QVariantMap leftoverTotals(const QDate& date) const;

  // Set leftover amounts (undoable) - supports partial allocation
  Q_INVOKABLE void setLeftoverAmounts(const QString& categoryName, const QDate& date,
                                      double saveAmount, double reportAmount);

public slots:
  void refresh();

signals:
  void countChanged();
  void leftoverDataChanged();

private:
  QList<Category*> _categories;
  BudgetData& _budgetData;
  const NavigationController& _navigation;
  QUndoStack& _undoStack;
};
