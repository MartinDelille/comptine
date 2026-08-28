#pragma once

#include <QtQml/qqml.h>
#include <QList>

#include "RuleListModel.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "model/PropertyMacros.h"
#include "model/Rule.h"

class BudgetData;
class QUndoStack;

class RuleController : public QObject {
  Q_OBJECT
  QML_ELEMENT

  PROPERTY_RO(int, ruleCount)

  Q_PROPERTY(RuleListModel* ruleModel READ ruleModel CONSTANT)

public:
  explicit RuleController(BudgetData& budgetData,
                          QUndoStack& undoStack);
  ~RuleController();

  // Rule access
  QList<Rule*> rules() const { return _rules; }
  Q_INVOKABLE Rule* at(int index) const;
  RuleListModel* ruleModel() { return _ruleModel; }

  // Rule management
  void addRule(Rule* rule);
  void clearRules();

  // For undo/redo support
  Rule* takeRule(int index);
  void moveRuleDirect(int fromIndex, int toIndex);

  // Matching operations
  Q_INVOKABLE const Category* matchingCategory(Operation* operation) const;

  // Apply rules to operations (used during import)
  int applyRulesToOperation(Operation* operation);

  // Apply a specific rule to all uncategorized operations (used after creating a new rule)

  // Navigation between uncategorized operations (for OperationEditDialog)
  Q_INVOKABLE Operation* nextUncategorizedOperation(Operation* current) const;
  Q_INVOKABLE Operation* previousUncategorizedOperation(Operation* current) const;

signals:
  void rulesChanged();

private:
  QList<Rule*> _rules;
  RuleListModel* _ruleModel = nullptr;
  BudgetData& _budgetData;
  QUndoStack& _undoStack;
};
