#pragma once

#include <QList>
#include <QObject>
#include <QQmlEngine>

#include "PropertyMacros.h"
#include "RuleListModel.h"

class BudgetData;
class CategorizationRule;
class Operation;
class QUndoStack;

class RuleController : public QObject {
  Q_OBJECT
  QML_ELEMENT

  PROPERTY_RO(int, ruleCount)
  PROPERTY_RO(int, uncategorizedCount)

  Q_PROPERTY(RuleListModel* ruleModel READ ruleModel CONSTANT)

public:
  explicit RuleController(QObject* parent = nullptr);
  ~RuleController();

  // Setup
  void setBudgetData(BudgetData* budgetData);
  void setUndoStack(QUndoStack* undoStack);

  // Rule access
  QList<CategorizationRule*> rules() const { return _rules; }
  Q_INVOKABLE CategorizationRule* getRule(int index) const;
  RuleListModel* ruleModel() { return _ruleModel; }

  // Rule management
  void addRule(CategorizationRule* rule);
  Q_INVOKABLE void addRule(const QString& category, const QString& descriptionPrefix);
  Q_INVOKABLE void removeRule(int index);
  Q_INVOKABLE void editRule(int index, const QString& category, const QString& descriptionPrefix);
  Q_INVOKABLE void moveRule(int fromIndex, int toIndex);
  void clearRules();

  // For undo/redo support
  CategorizationRule* takeRule(int index);
  void moveRuleDirect(int fromIndex, int toIndex);

  // Matching operations
  Q_INVOKABLE QString matchingCategory(Operation* operation) const;
  Q_INVOKABLE QString matchingCategoryForDescription(const QString& description) const;

  // Apply rules to operations (used during import)
  int applyRulesToOperation(Operation* operation);

  // Get uncategorized operations for the Categorize dialog
  Q_INVOKABLE QList<Operation*> uncategorizedOperations() const;
  Q_INVOKABLE Operation* nextUncategorizedOperation() const;

signals:
  void rulesChanged();

private:
  void updateUncategorizedCount();

  QList<CategorizationRule*> _rules;
  RuleListModel* _ruleModel = nullptr;
  BudgetData* _budgetData = nullptr;
  QUndoStack* _undoStack = nullptr;
};
