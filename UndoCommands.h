#pragma once

#include <QDate>
#include <QList>
#include <QString>
#include <QUndoCommand>

#include <optional>

#include "Category.h"   // For MonthRecord
#include "Operation.h"  // For Allocation

class Account;
class AccountListModel;
class BudgetData;
class Category;
class Rule;
class CategoryController;
class OperationListModel;
class RuleController;

class UndoStack : public QUndoStack {
  Q_OBJECT
  QML_ELEMENT
};

// Command for adding a new account
// Note: This command manages account lifecycle in BudgetData
class AddAccountCommand : public QUndoCommand {
public:
  AddAccountCommand(Account* account, BudgetData* budgetData,
                    QUndoCommand* parent = nullptr);
  ~AddAccountCommand();

  void undo() override;
  void redo() override;

  Account* account() const { return _account; }

private:
  Account* _account;
  BudgetData* _budgetData;
  bool _ownsAccount;  // True when account is not in BudgetData (after undo)
};

// Command for renaming an account
class RenameAccountCommand : public QUndoCommand {
public:
  RenameAccountCommand(Account& account, AccountListModel* accountModel,
                       const QString& oldName, const QString& newName,
                       QUndoCommand* parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Account& _account;
  AccountListModel* _accountModel;
  QString _oldName;
  QString _newName;
};

// Command for editing a category (name and/or budget limit)
// When the budget limit changes, the old limit is recorded in month_history
// for the month BEFORE the budget month (the last month the old limit was effective).
// This way, the budget month and all months after it show the new limit.
class EditCategoryCommand : public QUndoCommand {
public:
  EditCategoryCommand(Category& category,
                      const QString& oldName, const QString& newName,
                      double oldBudgetLimit, double newBudgetLimit,
                      const QDate& budgetDate,
                      QUndoCommand* parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Category& _category;
  QString _oldName;
  QString _newName;
  double _oldBudgetLimit;
  double _newBudgetLimit;
  QDate _budgetDate;                                  // The budget month when the change was made
  QDate _historyDate;                                 // The month before budgetDate (where old limit is stored)
  std::optional<double> _previousHistoryBudgetLimit;  // What was in month_history before (to restore on undo)
};

// Command for adding a single category
// Note: This command manages category lifecycle in CategoryController
class AddCategoryCommand : public QUndoCommand {
public:
  AddCategoryCommand(CategoryController* categoryController, Category* category,
                     QUndoCommand* parent = nullptr);
  ~AddCategoryCommand();

  void undo() override;
  void redo() override;

private:
  CategoryController* _categoryController;
  Category* _category;
  bool _ownsCategory;  // True when not in controller (after undo)
};

class DeleteCategoryCommand : public QUndoCommand {
public:
  DeleteCategoryCommand(
      CategoryController* categoryController,
      Category* category,
      QUndoCommand* parent = nullptr);
  ~DeleteCategoryCommand();

  void undo() override;
  void redo() override;

private:
  CategoryController* _categoryController;
  Category* _category;
  bool _ownsCategory;  // True when not in controller (after undo)
};

// Command for adding operation into an account
// Note: This command only manages operations. Use as child of a macro command
// with AddAccountCommand and AddCategoriesCommand for full import functionality.
class ImportOperationsCommand : public QUndoCommand {
public:
  ImportOperationsCommand(Account& account,
                          OperationListModel& operationModel,
                          const QList<Operation*>& operations,
                          QUndoCommand* parent = nullptr);
  ~ImportOperationsCommand();

  void undo() override;
  void redo() override;

private:
  Account& _account;
  OperationListModel& _operationModel;
  QList<Operation*> _operations;
  bool _ownsOperations;  // True when operations are not in the account (after undo)
};

// Command for adding an operation
class AddOperationCommand : public QUndoCommand {
public:
  AddOperationCommand(Operation* operation,
                      Account& account,
                      OperationListModel& operationModel,
                      QUndoCommand* parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Operation* _operation;
  Account& _account;
  OperationListModel& _operationModel;
};

// Command for deleting an operation
class DeleteOperationCommand : public QUndoCommand {
public:
  DeleteOperationCommand(Operation* operation,
                         Account& account,
                         OperationListModel& operationModel,
                         QUndoCommand* parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Operation* _operation;
  Account& _account;
  OperationListModel& _operationModel;
};

// Command for setting an operation's budget date
class SetOperationBudgetDateCommand : public QUndoCommand {
public:
  SetOperationBudgetDateCommand(Operation& operation,
                                OperationListModel* operationModel,
                                const QDate& oldBudgetDate,
                                const QDate& newBudgetDate,
                                QUndoCommand* parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Operation& _operation;
  OperationListModel* _operationModel;
  QDate _oldBudgetDate;
  QDate _newBudgetDate;
};

// Command for splitting an operation across multiple categories
class SplitOperationCommand : public QUndoCommand {
public:
  SplitOperationCommand(Operation& operation,
                        OperationListModel* operationModel,
                        const QList<Allocation*>& newAllocations,
                        QUndoCommand* parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Operation& _operation;
  OperationListModel* _operationModel;
  QList<Allocation*> _oldAllocations, _newAllocations;
};

// Command for setting an operation's amount
class SetOperationAmountCommand : public QUndoCommand {
public:
  SetOperationAmountCommand(Operation& operation,
                            OperationListModel* operationModel,
                            double oldAmount, double newAmount,
                            QUndoCommand* parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Operation& _operation;
  OperationListModel* _operationModel;
  double _oldAmount;
  double _newAmount;
};

// Command for setting an operation's date
class SetOperationDateCommand : public QUndoCommand {
public:
  SetOperationDateCommand(Operation& operation,
                          OperationListModel* operationModel,
                          const QDate& oldDate, const QDate& newDate,
                          QUndoCommand* parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Operation& _operation;
  OperationListModel* _operationModel;
  QDate _oldDate;
  QDate _newDate;
};

// Command for setting an operation's label
class SetOperationLabelCommand : public QUndoCommand {
public:
  SetOperationLabelCommand(Operation& operation,
                           OperationListModel* operationModel,
                           const QString& oldLabel,
                           const QString& newLabel,
                           QUndoCommand* parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Operation& _operation;
  OperationListModel* _operationModel;
  QString _oldLabel;
  QString _newLabel;
};

// Command for setting an operation's details
class SetOperationDetailsCommand : public QUndoCommand {
public:
  SetOperationDetailsCommand(Operation& operation,
                             OperationListModel* operationModel,
                             const QString& oldDetails,
                             const QString& newDetails,
                             QUndoCommand* parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Operation& _operation;
  OperationListModel* _operationModel;
  QString _oldDetails;
  QString _newDetails;
};

// Command for setting a category's leftover decision
class SetLeftoverDecisionCommand : public QUndoCommand {
public:
  SetLeftoverDecisionCommand(Category& category,
                             CategoryController* categoryController,
                             const QDate& date,
                             const LeftoverDecision& oldDecision,
                             const LeftoverDecision& newDecision,
                             QUndoCommand* parent = nullptr);

  void undo() override;
  void redo() override;
  int id() const override;
  bool mergeWith(const QUndoCommand* other) override;

private:
  Category& _category;
  CategoryController* _categoryController;
  QDate _date;
  LeftoverDecision _oldDecision;
  LeftoverDecision _newDecision;
};

// Command for adding a categorization rule
class AddRuleCommand : public QUndoCommand {
public:
  AddRuleCommand(RuleController* ruleController, Rule* rule,
                 QUndoCommand* parent = nullptr);
  ~AddRuleCommand();

  void undo() override;
  void redo() override;

private:
  RuleController* _ruleController;
  Rule* _rule;
  bool _ownsRule;
};

// Command for removing a categorization rule
class RemoveRuleCommand : public QUndoCommand {
public:
  RemoveRuleCommand(RuleController* ruleController, int index,
                    QUndoCommand* parent = nullptr);
  ~RemoveRuleCommand();

  void undo() override;
  void redo() override;

private:
  RuleController* _ruleController;
  Rule* _rule;
  int _index;
  bool _ownsRule;
};

// Command for editing a categorization rule
class EditRuleCommand : public QUndoCommand {
public:
  EditRuleCommand(RuleController* ruleController, int index,
                  const Category* oldCategory, const Category* newCategory,
                  const QString& oldLabelPrefix, const QString& newLabelPrefix,
                  double oldAmountFilter, double newAmountFilter,
                  QUndoCommand* parent = nullptr);

  void undo() override;
  void redo() override;

private:
  RuleController* _ruleController;
  int _index;
  const Category* _oldCategory;
  const Category* _newCategory;
  QString _oldLabelPrefix;
  QString _newLabelPrefix;
  double _oldAmountFilter;
  double _newAmountFilter;
};

// Command for moving a categorization rule (reordering priority)
class MoveRuleCommand : public QUndoCommand {
public:
  MoveRuleCommand(RuleController* ruleController, int fromIndex, int toIndex,
                  QUndoCommand* parent = nullptr);

  void undo() override;
  void redo() override;

private:
  RuleController* _ruleController;
  int _fromIndex;
  int _toIndex;
};
