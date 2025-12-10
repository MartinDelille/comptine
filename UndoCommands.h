#pragma once

#include <QDate>
#include <QList>
#include <QString>
#include <QUndoCommand>
#include "Operation.h"  // For CategoryAllocation

class BudgetData;
class Account;
class AccountListModel;
class Category;
class OperationListModel;

// Command for renaming an account
class RenameAccountCommand : public QUndoCommand {
public:
  RenameAccountCommand(Account *account, AccountListModel *accountModel,
                       const QString &oldName, const QString &newName,
                       QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Account *_account;
  AccountListModel *_accountModel;
  QString _oldName;
  QString _newName;
};

// Command for editing a category (name and/or budget limit)
class EditCategoryCommand : public QUndoCommand {
public:
  EditCategoryCommand(Category *category, BudgetData *budgetData,
                      const QString &oldName, const QString &newName,
                      double oldBudgetLimit, double newBudgetLimit,
                      QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  void renameOperationsCategory(const QString &fromName, const QString &toName);

  Category *_category;
  BudgetData *_budgetData;
  QString _oldName;
  QString _newName;
  double _oldBudgetLimit;
  double _newBudgetLimit;
};

// Command for importing operations from CSV
class ImportOperationsCommand : public QUndoCommand {
public:
  ImportOperationsCommand(Account *account, OperationListModel *operationModel,
                          BudgetData *budgetData,
                          const QList<Operation *> &operations,
                          const QList<Category *> &newCategories,
                          QUndoCommand *parent = nullptr);
  ~ImportOperationsCommand();

  void undo() override;
  void redo() override;

private:
  Account *_account;
  OperationListModel *_operationModel;
  BudgetData *_budgetData;
  QList<Operation *> _operations;
  QList<Category *> _newCategories;
  bool _ownsOperations;  // True when operations are not in the account (after undo)
  bool _ownsCategories;  // True when categories are not in budgetData (after undo)
};

// Command for setting an operation's category
class SetOperationCategoryCommand : public QUndoCommand {
public:
  SetOperationCategoryCommand(Operation *operation,
                              OperationListModel *operationModel,
                              BudgetData *budgetData,
                              const QString &oldCategory,
                              const QString &newCategory,
                              QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Operation *_operation;
  OperationListModel *_operationModel;
  BudgetData *_budgetData;
  QString _oldCategory;
  QString _newCategory;
};

// Command for setting an operation's budget date
class SetOperationBudgetDateCommand : public QUndoCommand {
public:
  SetOperationBudgetDateCommand(Operation *operation,
                                OperationListModel *operationModel,
                                BudgetData *budgetData,
                                const QDate &oldBudgetDate,
                                const QDate &newBudgetDate,
                                QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Operation *_operation;
  OperationListModel *_operationModel;
  BudgetData *_budgetData;
  QDate _oldBudgetDate;
  QDate _newBudgetDate;
};

// Command for splitting an operation across multiple categories
class SplitOperationCommand : public QUndoCommand {
public:
  SplitOperationCommand(Operation *operation,
                        OperationListModel *operationModel,
                        BudgetData *budgetData,
                        const QString &oldCategory,
                        const QList<CategoryAllocation> &oldAllocations,
                        const QList<CategoryAllocation> &newAllocations,
                        QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Operation *_operation;
  OperationListModel *_operationModel;
  BudgetData *_budgetData;
  QString _oldCategory;
  QList<CategoryAllocation> _oldAllocations;
  QList<CategoryAllocation> _newAllocations;
};

// Command for setting an operation's amount
class SetOperationAmountCommand : public QUndoCommand {
public:
  SetOperationAmountCommand(Operation *operation,
                            OperationListModel *operationModel,
                            BudgetData *budgetData, double oldAmount,
                            double newAmount,
                            QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Operation *_operation;
  OperationListModel *_operationModel;
  BudgetData *_budgetData;
  double _oldAmount;
  double _newAmount;
};

// Command for setting an operation's date
class SetOperationDateCommand : public QUndoCommand {
public:
  SetOperationDateCommand(Operation *operation,
                          OperationListModel *operationModel,
                          BudgetData *budgetData, const QDate &oldDate,
                          const QDate &newDate,
                          QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  Operation *_operation;
  OperationListModel *_operationModel;
  BudgetData *_budgetData;
  QDate _oldDate;
  QDate _newDate;
};
