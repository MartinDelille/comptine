// Unit tests for Category class
#include <QDate>
#include <QSignalSpy>
#include <QTest>
#include <QVariant>

#include "editor/OperationEditor.h"
#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "services/BudgetData.h"
#include "services/CategoryController.h"
#include "services/UndoCommands.h"

class CategoryTest : public QObject {
  Q_OBJECT

  QUndoStack* undoStack;
  BudgetData* budgetData;
  CategoryController* categoryController;
  OperationEditor* operationEditor;

private slots:
  void init() {
    // Create fresh instances before each test
    undoStack = new QUndoStack();  // No parent - we'll delete manually
    budgetData = new BudgetData(*undoStack);
    categoryController = new CategoryController(*budgetData, *undoStack);
    operationEditor = new OperationEditor(*budgetData, *undoStack);
  }

  void cleanup() {
    delete categoryController;
    delete operationEditor;
    delete budgetData;
    delete undoStack;
  }

  // YearMonth struct tests

  void testYearMonthLessThan() {
    QVERIFY(YearMonth({ 2025, 1 }) < YearMonth({ 2025, 2 }));
    QVERIFY(YearMonth({ 2024, 12 }) < YearMonth({ 2025, 1 }));
    QVERIFY(!(YearMonth({ 2025, 3 }) < YearMonth({ 2025, 3 })));
    QVERIFY(!(YearMonth({ 2025, 6 }) < YearMonth({ 2025, 1 })));
  }

  void testYearMonthEqual() {
    QVERIFY(YearMonth({ 2025, 1 }) == YearMonth({ 2025, 1 }));
    QVERIFY(!(YearMonth({ 2025, 1 }) == YearMonth({ 2025, 2 })));
    QVERIFY(!(YearMonth({ 2024, 1 }) == YearMonth({ 2025, 1 })));
  }

  void testYearMonthLessThanOrEqual() {
    QVERIFY(YearMonth({ 2025, 1 }) <= YearMonth({ 2025, 1 }));
    QVERIFY(YearMonth({ 2025, 1 }) <= YearMonth({ 2025, 2 }));
    QVERIFY(!(YearMonth({ 2025, 3 }) <= YearMonth({ 2025, 2 })));
  }

  void testYearMonthFromDate() {
    auto ym = YearMonth::fromDate(QDate(2025, 6, 15));
    QCOMPARE(ym.year, 2025);
    QCOMPARE(ym.month, 6);
  }

  // MonthRecord struct tests

  void testMonthRecordIsEmpty() {
    MonthRecord record;
    QVERIFY(record.isEmpty());

    record.saveAmount = 10.0;
    QVERIFY(!record.isEmpty());

    record.saveAmount = 0.0;
    record.reportAmount = 5.0;
    QVERIFY(!record.isEmpty());

    record.reportAmount = 0.0;
    record.budgetLimit = 100.0;
    QVERIFY(!record.isEmpty());
  }

  void testMonthRecordHasLeftoverData() {
    MonthRecord record;
    QVERIFY(!record.hasLeftoverData());

    record.saveAmount = 10.0;
    QVERIFY(record.hasLeftoverData());

    record.saveAmount = 0.0;
    record.reportAmount = 5.0;
    QVERIFY(record.hasLeftoverData());

    // budgetLimit alone doesn't count as leftover data
    record.reportAmount = 0.0;
    record.budgetLimit = 100.0;
    QVERIFY(!record.hasLeftoverData());
  }

  void testMonthRecordLeftoverTotal() {
    MonthRecord record;
    record.saveAmount = 100.0;
    record.reportAmount = 50.0;
    QCOMPARE(record.leftoverTotal(), 150.0);
  }

  // Category constructor tests

  void testDefaultConstructor() {
    Category cat;
    QCOMPARE(cat.name(), QString());
  }

  void testParameterizedConstructor() {
    Category cat("Groceries");
    QCOMPARE(cat.name(), QString("Groceries"));
  }

  // Month history management

  void testSetAndGetMonthRecord() {
    Category cat("Test");

    MonthRecord record;
    record.saveAmount = 50.0;
    record.reportAmount = 25.0;
    record.budgetLimit = 200.0;
    cat.setMonthRecord(2025, 6, record);

    MonthRecord retrieved = cat.monthRecord(2025, 6);
    QCOMPARE(retrieved.saveAmount, 50.0);
    QCOMPARE(retrieved.reportAmount, 25.0);
    QVERIFY(retrieved.budgetLimit.has_value());
    QCOMPARE(retrieved.budgetLimit.value(), 200.0);
  }

  void testGetNonExistentMonthRecord() {
    Category cat("Test");

    MonthRecord record = cat.monthRecord(2025, 1);
    QVERIFY(record.isEmpty());
    QCOMPARE(record.saveAmount, 0.0);
    QCOMPARE(record.reportAmount, 0.0);
    QVERIFY(!record.budgetLimit.has_value());
  }

  void testSetEmptyMonthRecordRemovesEntry() {
    Category cat("Test");

    // Set a non-empty record
    cat.setMonthRecord(2025, 6, { 50.0, 25.0 });
    QVERIFY(!cat.monthRecord(2025, 6).isEmpty());

    // Set an empty record — should remove the entry
    cat.setMonthRecord(2025, 6, MonthRecord{});
    QVERIFY(cat.allMonthHistory().isEmpty());
  }

  void testClearMonthRecord() {
    Category cat("Test");

    cat.setMonthRecord(2025, 6, { 50.0, 25.0 });
    QCOMPARE(cat.allMonthHistory().size(), 1);

    cat.clearMonthRecord(2025, 6);
    QVERIFY(cat.allMonthHistory().isEmpty());
  }

  void testClearNonExistentMonthRecordNoSignal() {
    Category cat("Test");

    QSignalSpy spy(&cat, &Category::monthHistoryChanged);
    cat.clearMonthRecord(2025, 6);  // Nothing to clear
    QCOMPARE(spy.count(), 0);
  }

  void testMonthHistoryChangedSignal() {
    Category cat("Test");

    QSignalSpy spy(&cat, &Category::monthHistoryChanged);
    cat.setMonthRecord(2025, 6, { 50.0, 25.0 });

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args[0].toInt(), 2025);
    QCOMPARE(args[1].toInt(), 6);
  }

  // Legacy leftover decision wrappers

  void testSetLeftoverDecisionPreservesBudgetLimit() {
    Category cat("Test");

    // First set a budget limit for this month
    cat.setBudgetLimitForMonth(2025, 6, 200.0);

    // Now set leftover decision — should preserve the budget limit
    cat.setLeftoverDecision(2025, 6, { 50.0, 25.0 });

    MonthRecord record = cat.monthRecord(2025, 6);
    QCOMPARE(record.saveAmount, 50.0);
    QCOMPARE(record.reportAmount, 25.0);
    QVERIFY(record.budgetLimit.has_value());
    QCOMPARE(record.budgetLimit.value(), 200.0);
  }

  void testClearLeftoverDecisionPreservesBudgetLimit() {
    Category cat("Test");

    // Set both leftover data and budget limit
    MonthRecord record;
    record.saveAmount = 50.0;
    record.reportAmount = 25.0;
    record.budgetLimit = 200.0;
    cat.setMonthRecord(2025, 6, record);

    // Clear leftover — budget limit should remain
    cat.clearLeftoverDecision(2025, 6);

    MonthRecord result = cat.monthRecord(2025, 6);
    QCOMPARE(result.saveAmount, 0.0);
    QCOMPARE(result.reportAmount, 0.0);
    QVERIFY(result.budgetLimit.has_value());
    QCOMPARE(result.budgetLimit.value(), 200.0);
  }

  void testClearLeftoverDecisionRemovesEntryWhenNoBudgetLimit() {
    Category cat("Test");

    // Set only leftover data (no budget limit)
    cat.setLeftoverDecision(2025, 6, { 50.0, 25.0 });

    // Clear leftover — entry should be removed entirely
    cat.clearLeftoverDecision(2025, 6);
    QVERIFY(cat.allMonthHistory().isEmpty());
  }

  // Budget limit for month (the core algorithm)

  void testBudgetLimitForMonthNoHistory() {
    Category cat("Groceries");

    // No history at all — no budget is defined yet.
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 1, 1)), 0.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 6, 15)), 0.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2026, 12, 1)), 0.0);
  }

  void testBudgetLimitForMonthWithSingleHistoryEntry() {
    // A history entry is effective from its own month.
    Category cat("Groceries");
    cat.setBudgetLimitForMonth(2025, 6, -250.0);

    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 1, 1)), 0.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 6, 15)), -250.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 7, 1)), -250.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 12, 1)), -250.0);
  }

  void testBudgetLimitForMonthWithMultipleHistoryEntries() {
    // Multiple entries select the latest value at or before the requested month.
    Category cat("Groceries");
    cat.setBudgetLimitForMonth(2025, 3, -200.0);
    cat.setBudgetLimitForMonth(2025, 6, -250.0);

    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 1, 1)), 0.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 3, 1)), -200.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 4, 1)), -200.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 6, 1)), -250.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 7, 1)), -250.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 12, 1)), -250.0);
  }

  void testBudgetLimitForMonthIgnoresLeftoverOnlyEntries() {
    // History entries without budgetLimit should be skipped
    Category cat("Groceries");
    cat.setBudgetLimitForMonth(2025, 3, -200.0);

    // Add a leftover-only entry in April (no budgetLimit)
    cat.setLeftoverDecision(2025, 4, { 50.0, 25.0 });

    // The leftover entry at April should not affect the budget limit lookup.
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 1, 1)), 0.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 3, 1)), -200.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 4, 1)), -200.0);
  }

  void testBudgetLimitForMonthCrossYearBoundary() {
    Category cat("Groceries");
    cat.setBudgetLimitForMonth(2024, 11, -300.0);  // Nov 2024 was the last month at 300

    QCOMPARE(cat.budgetLimitForMonth(QDate(2024, 6, 1)), 0.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2024, 11, 1)), -300.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2024, 12, 1)), -300.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 1, 1)), -300.0);
  }

  // setBudgetLimitForMonth / clearBudgetLimitForMonth

  void testSetBudgetLimitForMonth() {
    Category cat("Test");

    QSignalSpy spy(&cat, &Category::monthHistoryChanged);
    cat.setBudgetLimitForMonth(2025, 6, 200.0);

    QCOMPARE(spy.count(), 1);
    MonthRecord record = cat.monthRecord(2025, 6);
    QVERIFY(record.budgetLimit.has_value());
    QCOMPARE(record.budgetLimit.value(), 200.0);
  }

  void testClearBudgetLimitForMonth() {
    Category cat("Test");
    cat.setBudgetLimitForMonth(2025, 6, 200.0);

    QSignalSpy spy(&cat, &Category::monthHistoryChanged);
    cat.clearBudgetLimitForMonth(2025, 6);

    QCOMPARE(spy.count(), 1);
    // Entry should be removed since it had no leftover data
    QVERIFY(cat.allMonthHistory().isEmpty());
  }

  void testClearBudgetLimitForMonthPreservesLeftoverData() {
    Category cat("Test");

    // Set both leftover data and budget limit
    MonthRecord record;
    record.saveAmount = 50.0;
    record.budgetLimit = 200.0;
    cat.setMonthRecord(2025, 6, record);

    cat.clearBudgetLimitForMonth(2025, 6);

    // Leftover data should remain
    MonthRecord result = cat.monthRecord(2025, 6);
    QCOMPARE(result.saveAmount, 50.0);
    QVERIFY(!result.budgetLimit.has_value());
  }

  // Accumulated leftover

  void testAccumulatedLeftoverBefore() {
    Category cat("Test");

    cat.setLeftoverDecision(2025, 1, { 0.0, 30.0 });   // report 30
    cat.setLeftoverDecision(2025, 2, { 50.0, 20.0 });  // report 20
    cat.setLeftoverDecision(2025, 3, { 0.0, 10.0 });   // report 10

    // Before January: nothing
    QCOMPARE(cat.accumulatedLeftoverBefore(QDate(2024, 12, 31)), 0.0);

    // Before February: only January's report
    QCOMPARE(cat.accumulatedLeftoverBefore(QDate(2025, 1, 31)), 30.0);

    // Before March: January + February reports
    QCOMPARE(cat.accumulatedLeftoverBefore(QDate(2025, 2, 28)), 50.0);

    // Before April: all three
    QCOMPARE(cat.accumulatedLeftoverBefore(QDate(2025, 3, 31)), 60.0);
  }

  void testAccumulatedLeftoverBeforeIgnoresSaveAmounts() {
    Category cat("Test");

    // Only save, no report
    cat.setLeftoverDecision(2025, 1, { 100.0, 0.0 });

    QCOMPARE(cat.accumulatedLeftoverBefore(QDate(2025, 2, 1)), 0.0);
  }

  void testAccumulatedLeftoverBeforeCrossYear() {
    Category cat("Test");

    cat.setLeftoverDecision(2024, 11, { 0.0, 40.0 });
    cat.setLeftoverDecision(2024, 12, { 0.0, 25.0 });
    cat.setLeftoverDecision(2025, 1, { 0.0, 15.0 });

    // Before Feb 2025: all three
    QCOMPARE(cat.accumulatedLeftoverBefore(QDate(2025, 2, 1)), 80.0);

    // Before December 2024: only 2024 entries
    QCOMPARE(cat.accumulatedLeftoverBefore(QDate(2024, 12, 1)), 65.0);
  }

  // allMonthHistory

  void testAllMonthHistory() {
    Category cat("Test");

    cat.setMonthRecord(2025, 1, { 10.0, 5.0 });
    cat.setMonthRecord(2025, 3, { 20.0, 10.0 });
    cat.setBudgetLimitForMonth(2025, 6, 200.0);

    auto history = cat.allMonthHistory();
    QCOMPARE(history.size(), 3);
    QVERIFY(history.contains({ 2025, 1 }));
    QVERIFY(history.contains({ 2025, 3 }));
    QVERIFY(history.contains({ 2025, 6 }));
  }
  // EditCategoryCommand undo/redo with budget limit history

  void testEditCategoryCommandUndoRedo() {
    auto cat = new Category("Food");
    categoryController->addCategory(cat);
    cat->setBudgetLimitForMonth(2025, 1, -250.0);

    QDate budgetDate(2025, 6, 1);  // Changing budget while viewing June

    // Push edit command: change limit from 250 to 300
    undoStack->push(new EditCategoryCommand(*cat, "Food",
                                            -300.0, budgetDate));

    // The new limit is stored directly at June.
    QCOMPARE(cat->budgetLimitForMonth(QDate(2025, 5, 1)), -250.0);
    QCOMPARE(cat->budgetLimitForMonth(QDate(2025, 6, 1)), -300.0);
    QCOMPARE(cat->budgetLimitForMonth(QDate(2025, 7, 1)), -300.0);

    // Undo should clear the June history entry.
    undoStack->undo();
    MonthRecord record = cat->monthRecord(2025, 6);
    QVERIFY(!record.budgetLimit.has_value());

    // All months should now return the January limit.
    QCOMPARE(cat->budgetLimitForMonth(QDate(2025, 5, 1)), -250.0);
    QCOMPARE(cat->budgetLimitForMonth(QDate(2025, 6, 1)), -250.0);
    QCOMPARE(cat->budgetLimitForMonth(QDate(2025, 7, 1)), -250.0);

    // Redo again
    undoStack->redo();
    QCOMPARE(cat->budgetLimitForMonth(QDate(2025, 5, 1)), -250.0);
    QCOMPARE(cat->budgetLimitForMonth(QDate(2025, 6, 1)), -300.0);
    QCOMPARE(cat->budgetLimitForMonth(QDate(2025, 7, 1)), -300.0);
  }

  void testEditCategoryCommandPreservesExistingHistory() {
    auto cat = new Category("Food");
    categoryController->addCategory(cat);
    cat->setBudgetLimitForMonth(2025, 1, -250.0);

    // Pre-existing budget limit in history for May (e.g., from a previous change)
    cat->setBudgetLimitForMonth(2025, 5, -200.0);

    QDate budgetDate(2025, 6, 1);

    // Change limit from 250 to 300 while viewing June.
    undoStack->push(new EditCategoryCommand(*cat, "Food",
                                            -300.0, budgetDate));

    // The existing May override remains unchanged.
    QCOMPARE(cat->budgetLimitForMonth(QDate(2025, 5, 1)), -200.0);
    // June should show new limit
    QCOMPARE(cat->budgetLimitForMonth(QDate(2025, 6, 1)), -300.0);

    // Undo restores the pre-existing May value and removes the June override.
    undoStack->undo();
    MonthRecord record = cat->monthRecord(2025, 6);
    QVERIFY(!record.budgetLimit.has_value());
    QCOMPARE(cat->budgetLimitForMonth(QDate(2025, 5, 1)), -200.0);
  }

  void testEditCategoryCommandPreservesLaterBudgetLimit() {
    auto cat = new Category("Food");
    categoryController->addCategory(cat);
    cat->setBudgetLimitForMonth(2025, 1, -250.0);

    undoStack->push(new EditCategoryCommand(*cat, "Food", -300.0,
                                            QDate(2025, 4, 1)));
    undoStack->push(new EditCategoryCommand(*cat, "Food", -400.0,
                                            QDate(2025, 3, 1)));

    QCOMPARE(cat->budgetLimitForMonth(QDate(2025, 3, 1)), -400.0);
    QCOMPARE(cat->budgetLimitForMonth(QDate(2025, 4, 1)), -300.0);

    undoStack->undo();
    QCOMPARE(cat->budgetLimitForMonth(QDate(2025, 3, 1)), -250.0);
    QCOMPARE(cat->budgetLimitForMonth(QDate(2025, 4, 1)), -300.0);
  }

  void testEditCategoryNameOnlyNoHistoryChange() {
    auto cat = new Category("Food");
    categoryController->addCategory(cat);

    QDate budgetDate(2025, 6, 1);

    // Only rename, don't change budget limit
    undoStack->push(new EditCategoryCommand(*cat, "Groceries",
                                            std::nullopt, budgetDate));

    // Name should change, but no history entry should be created
    QCOMPARE(cat->name(), QString("Groceries"));
    QCOMPARE(cat->budgetLimitForMonth(budgetDate), 0.0);
    QVERIFY(cat->allMonthHistory().isEmpty());

    undoStack->undo();
    QCOMPARE(cat->name(), QString("Food"));
  }

  void testCountOperationsWithCategory() {
    auto food = categoryController->addCategory(new Category("Food"));
    auto transport = categoryController->addCategory(new Category("Transport"));
    auto account = budgetData->createAccount("Account");
    auto bread = account->addOperation(new Operation(account, QDate(2026, 8, 15), 1., "Bread", "", { new Allocation(food, 1) }));
    QCOMPARE(budgetData->countOperationsWithCategory(food), 1);
    QCOMPARE(budgetData->countOperationsWithCategory(transport), 0);
  }

  void testSplitOperationUndoRedoPreservesAllocations() {
    auto food = categoryController->addCategory(new Category("Food"));
    auto transport = categoryController->addCategory(new Category("Transport"));
    auto account = budgetData->createAccount("Account");
    auto operation = account->addOperation(new Operation(account, QDate(2026, 8, 15), -100., "Purchase"));

    QVERIFY(operationEditor->beginEditing(operation));
    operationEditor->addAllocation(operation, food, -60.);
    operationEditor->addAllocation(operation, transport, -40.);
    operationEditor->endEditing(true);
    QCOMPARE(operation->allocations().size(), 2);
    QCOMPARE(operation->amountForCategory(food), -60.);
    QCOMPARE(operation->amountForCategory(transport), -40.);

    undoStack->undo();
    QVERIFY(operation->allocations().isEmpty());

    undoStack->redo();
    QCOMPARE(operation->allocations().size(), 2);
    QCOMPARE(operation->amountForCategory(food), -60.);
    QCOMPARE(operation->amountForCategory(transport), -40.);

    undoStack->undo();
    QVERIFY(operation->allocations().isEmpty());
  }

  void testOperationAllocationModelAndTransactionalCancel() {
    auto food = categoryController->addCategory(new Category("Food"));
    auto account = budgetData->createAccount("Account");
    auto operation = account->addOperation(new Operation(account, QDate(2026, 8, 15), -100., "Purchase"));

    QVERIFY(operationEditor->beginEditing(operation));
    operationEditor->addAllocation(operation, food, -100.);
    QCOMPARE(operation->rowCount(), 1);
    QCOMPARE(operation->data(operation->index(0, 0), Operation::CategoryRole).value<Category*>(), food);
    QCOMPARE(operation->data(operation->index(0, 0), Operation::AmountRole).toDouble(), -100.);
    QCOMPARE(operation->allocatedAmount(), -100.);
    operationEditor->endEditing(false);

    QVERIFY(operation->allocations().isEmpty());
    QVERIFY(!operationEditor->isEditing());
  }

  void testAcceptedAllocationNormalizationIsOneUndoStep() {
    auto food = categoryController->addCategory(new Category("Food"));
    auto account = budgetData->createAccount("Account");
    auto operation = account->addOperation(new Operation(account, QDate(2026, 8, 15), -100., "Purchase"));
    operation->setAllocations({ new Allocation(food, -40.),
                                new Allocation(food, -20.),
                                new Allocation(food, 0.0001) });

    QVERIFY(operationEditor->beginEditing(operation));
    operationEditor->normalizeAllocations(operation);
    operationEditor->endEditing(true);

    QCOMPARE(operation->rowCount(), 1);
    QCOMPARE(operation->amountForCategory(food), -60.);
    undoStack->undo();
    QCOMPARE(operation->allocations().size(), 3);
    QCOMPARE(operation->amountForCategory(food), -59.9999);
  }

  void testChangingOperationDateResortsAccount() {
    auto account = budgetData->createAccount("Account");
    auto older = account->addOperation(new Operation(account, QDate(2026, 8, 15), -10., "Older"));
    auto newer = account->addOperation(new Operation(account, QDate(2026, 8, 20), -20., "Newer"));

    QCOMPARE(account->operationAt(0), newer);
    QCOMPARE(account->operationAt(1), older);

    operationEditor->setDate(older, QDate(2026, 8, 25));
    QCOMPARE(account->operationAt(0), older);
    QCOMPARE(account->operationAt(1), newer);

    undoStack->undo();
    QCOMPARE(account->operationAt(0), newer);
    QCOMPARE(account->operationAt(1), older);

    undoStack->redo();
    QCOMPARE(account->operationAt(0), older);
    QCOMPARE(account->operationAt(1), newer);
  }
};

QTEST_GUILESS_MAIN(CategoryTest)
#include "CategoryTest.moc"
