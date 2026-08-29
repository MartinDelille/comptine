#include "CategoryEditor.h"

#include "BudgetData.h"
#include "CategoryController.h"
#include "UndoCommands.h"
#include "model/Operation.h"

CategoryEditor::CategoryEditor(CategoryController& controller, BudgetData& budgetData,
                               QUndoStack& undoStack, QObject* parent) :
    QObject(parent), _controller(controller), _budgetData(budgetData), _undoStack(undoStack) {
}

Allocation* CategoryEditor::createAllocation(const QString& categoryName, double amount) {
  return new Allocation(_controller.getCategoryByName(categoryName), amount);
}

Category* CategoryEditor::edit(const QString& name, double budgetLimit,
                               Category* category, QDate budgetDate) {
  if (category) {
    if (category->name() != name || category->budgetLimitForMonth(budgetDate) != budgetLimit)
      _undoStack.push(new EditCategoryCommand(*category, name, budgetLimit, budgetDate));
    return category;
  }
  auto* newCategory = new Category(name, budgetLimit);
  _undoStack.push(new AddCategoryCommand(&_controller, newCategory));
  return newCategory;
}

void CategoryEditor::remove(Category* category) {
  if (!category) return;
  const int previousIndex = _controller.currentIndex();
  auto* macro = new QUndoCommand();
  for (auto* account : _budgetData.accounts()) {
    for (auto* operation : account->operations()) {
      QList<Allocation*> allocations;
      for (auto* allocation : operation->allocations()) {
        if (allocation && allocation->category() != category)
          allocations.append(new Allocation(allocation->category(), allocation->amount()));
      }
      if (allocations.size() != operation->allocations().size())
        new SplitOperationCommand(*operation, allocations, macro);
    }
  }
  _undoStack.push(new DeleteCategoryCommand(&_controller, category));
  _undoStack.push(macro);
  if (_controller.current() == category) _controller.set_currentIndex(previousIndex);
}

void CategoryEditor::setSaveAmount(Category* category, const QDate& date, double amount) {
  if (!category) return;
  auto oldRecord = category->monthRecord(date.year(), date.month());
  auto newRecord = oldRecord;
  newRecord.saveAmount = amount;
  if (!qFuzzyCompare(oldRecord.saveAmount, newRecord.saveAmount))
    _undoStack.push(new SetLeftoverDecisionCommand(*category, &_controller, date, newRecord));
}

void CategoryEditor::setReportAmount(Category* category, const QDate& date, double amount) {
  if (!category) return;
  auto oldRecord = category->monthRecord(date.year(), date.month());
  auto newRecord = oldRecord;
  newRecord.reportAmount = amount;
  if (!qFuzzyCompare(oldRecord.saveAmount, newRecord.saveAmount) || !qFuzzyCompare(oldRecord.reportAmount, newRecord.reportAmount))
    _undoStack.push(new SetLeftoverDecisionCommand(*category, &_controller, date, newRecord));
}
