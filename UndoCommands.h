#pragma once

#include <QDate>
#include <QList>
#include <QString>
#include <QUndoCommand>

#include "Category.h"   // For LeftoverDecision
#include "Operation.h"  // For CategoryAllocation

class Account;
class AccountListModel;
class BudgetData;
class Category;
class CategoryController;
class OperationListModel;

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
// Note: This command still needs BudgetData because it:
// 1. Iterates over all accounts to rename operations when category name changes
// 2. Emits categoryCountChanged to refresh the UI
class EditCategoryCommand : public QUndoCommand {
public:
  EditCategoryCommand(Category& category, BudgetData* budgetData, CategoryController* categoryController,
                      const QString& oldName, const QString& newName,
                      double oldBudgetLimit, double newBudgetLimit,
                      QUndoCommand* parent = nullptr);

  void undo() override;
  void redo() override;

private:
  void renameOperationsCategory(const QString& fromName, const QString& toName);

  Category& _category;
  BudgetData* _budgetData;
  CategoryController* _categoryController;
  QString _oldName;
  QString _newName;
  double _oldBudgetLimit;
  double _newBudgetLimit;
};

// Command for adding categories
// Note: This command manages category lifecycle in CategoryController
class AddCategoriesCommand : public QUndoCommand {
public:
  AddCategoriesCommand(CategoryController* categoryController,
                       const QList<Category*>& categories,
                       QUndoCommand* parent = nullptr);
  ~AddCategoriesCommand();

  void undo() override;
  void redo() override;

private:
  CategoryController* _categoryController;
  QList<Category*> _categories;
  bool _ownsCategories;  // True when categories are not in CategoryController (after undo)
};

// Command for importing operations into an account
// Note: This command only manages operations. Use as child of a macro command
// with AddAccountCommand and AddCategoriesCommand for full import functionality.
class ImportOperationsCommand : public QUndoCommand {
public:
  ImportOperationsCommand(Account* account, OperationListModel* operationModel,
                          const QList<Operation*>& operations,
                          QUndoCommand* parent = nullptr);
  ~ImportOperationsCommand();

  void undo() override;
  void redo() override;

private:
  Account* _account;
  OperationListModel* _operationModel;
  QList<Operation*> _operations;
  bool _ownsOperations;  // True when operations are not in the account (after undo)
};

// Command for setting an operation's category
class SetOperationCategoryCommand : public QUndoCommand {
public:
  SetOperationCategoryCommand(Operation& operation,
                              OperationListModel* operationModel,
                              const QString& oldCategory,
                              const QString& newCategory,
                              QUndoCommand* parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Operation& _operation;
  OperationListModel* _operationModel;
  QString _oldCategory;
  QString _newCategory;
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
                        const QString& oldCategory,
                        const QList<CategoryAllocation>& oldAllocations,
                        const QList<CategoryAllocation>& newAllocations,
                        QUndoCommand* parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Operation& _operation;
  OperationListModel* _operationModel;
  QString _oldCategory;
  QList<CategoryAllocation> _oldAllocations;
  QList<CategoryAllocation> _newAllocations;
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

// Command for setting an operation's description
class SetOperationDescriptionCommand : public QUndoCommand {
public:
  SetOperationDescriptionCommand(Operation& operation,
                                 OperationListModel* operationModel,
                                 const QString& oldDescription,
                                 const QString& newDescription,
                                 QUndoCommand* parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Operation& _operation;
  OperationListModel* _operationModel;
  QString _oldDescription;
  QString _newDescription;
};

// Command for setting a category's leftover decision
class SetLeftoverDecisionCommand : public QUndoCommand {
public:
  SetLeftoverDecisionCommand(Category& category,
                             CategoryController* categoryController,
                             int year, int month,
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
  int _year;
  int _month;
  LeftoverDecision _oldDecision;
  LeftoverDecision _newDecision;
};
