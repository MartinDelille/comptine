#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUndoStack>
#include <QVariant>

#include "Category.h"
#include "LeftoverListModel.h"
#include "PropertyMacros.h"

class Account;
class BudgetData;

class CategoryController : public QObject {
  Q_OBJECT

  PROPERTY_RO(int, categoryCount)
  Q_PROPERTY(LeftoverListModel* leftoverModel READ leftoverModel CONSTANT)

public:
  explicit CategoryController(BudgetData& budgetData,
                              QUndoStack& undoStack);

  // Leftover model accessor
  LeftoverListModel* leftoverModel() { return &_leftoverModel; }

  // Category accessors
  QList<Category*> categories() const;
  Q_INVOKABLE Category* getCategory(int index) const;
  Q_INVOKABLE Category* getCategoryByName(const QString& name) const;
  Q_INVOKABLE QStringList categoryNames() const;

  // Category management
  Q_INVOKABLE void addCategory(const QString& name, double budgetLimit);
  void addCategory(Category* category);
  void removeCategory(int index);
  void clearCategories();
  Category* takeCategoryByName(const QString& name);  // Remove without deleting

  // Category editing (undoable)
  Q_INVOKABLE void editCategory(const QString& originalName, const QString& newName, double newBudgetLimit);

  // Budget calculations (aggregates across all accounts)
  Q_INVOKABLE double spentInCategory(const Category* categoryName, int year, int month) const;
  Q_INVOKABLE QVariantList monthlyBudgetSummary(int year, int month) const;
  Q_INVOKABLE QVariantList operationsForCategory(const Category* category, int year, int month) const;

  // Leftover calculations
  // Calculate the leftover for a category in a month: budget - spent (+ accumulated from previous months)
  Q_INVOKABLE double leftoverForCategory(const Category* category, int year, int month) const;

  // Get accumulated leftover from previous months (sum of all "report" decisions before this month)
  Q_INVOKABLE double accumulatedLeftover(const QString& categoryName, int year, int month) const;

  // Get the current leftover decision for a category in a month
  Q_INVOKABLE QVariantMap leftoverDecision(const QString& categoryName, int year, int month) const;

  // Get summary of all categories with leftover info for a month (for the leftover dialog)
  Q_INVOKABLE QVariantList leftoverSummary(int year, int month) const;

  // Get monthly totals for savings and leftover transfers
  Q_INVOKABLE QVariantMap leftoverTotals(int year, int month) const;

  // Set leftover amounts (undoable) - supports partial allocation
  Q_INVOKABLE void setLeftoverAmounts(const QString& categoryName, int year, int month,
                                      double saveAmount, double reportAmount);

signals:
  void leftoverDataChanged();

private:
  QList<Category*> _categories;
  BudgetData& _budgetData;
  QUndoStack& _undoStack;
  LeftoverListModel _leftoverModel;
};
