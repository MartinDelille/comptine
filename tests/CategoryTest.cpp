// Unit tests for Category class
#include <QDate>
#include <QSignalSpy>
#include <QTest>

#include "../Category.h"

class CategoryTest : public QObject {
  Q_OBJECT

private slots:

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
    QCOMPARE(cat.budgetLimit(), 0.0);
  }

  void testParameterizedConstructor() {
    Category cat("Groceries", -300.0);
    QCOMPARE(cat.name(), QString("Groceries"));
    QCOMPARE(cat.budgetLimit(), -300.0);
  }

  // Month history management

  void testSetAndGetMonthRecord() {
    Category cat("Test", 100.0);

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
    Category cat("Test", 100.0);

    MonthRecord record = cat.monthRecord(2025, 1);
    QVERIFY(record.isEmpty());
    QCOMPARE(record.saveAmount, 0.0);
    QCOMPARE(record.reportAmount, 0.0);
    QVERIFY(!record.budgetLimit.has_value());
  }

  void testSetEmptyMonthRecordRemovesEntry() {
    Category cat("Test", 100.0);

    // Set a non-empty record
    cat.setMonthRecord(2025, 6, { 50.0, 25.0 });
    QVERIFY(!cat.monthRecord(2025, 6).isEmpty());

    // Set an empty record — should remove the entry
    cat.setMonthRecord(2025, 6, MonthRecord{});
    QVERIFY(cat.allMonthHistory().isEmpty());
  }

  void testClearMonthRecord() {
    Category cat("Test", 100.0);

    cat.setMonthRecord(2025, 6, { 50.0, 25.0 });
    QCOMPARE(cat.allMonthHistory().size(), 1);

    cat.clearMonthRecord(2025, 6);
    QVERIFY(cat.allMonthHistory().isEmpty());
  }

  void testClearNonExistentMonthRecordNoSignal() {
    Category cat("Test", 100.0);

    QSignalSpy spy(&cat, &Category::monthHistoryChanged);
    cat.clearMonthRecord(2025, 6);  // Nothing to clear
    QCOMPARE(spy.count(), 0);
  }

  void testMonthHistoryChangedSignal() {
    Category cat("Test", 100.0);

    QSignalSpy spy(&cat, &Category::monthHistoryChanged);
    cat.setMonthRecord(2025, 6, { 50.0, 25.0 });

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args[0].toInt(), 2025);
    QCOMPARE(args[1].toInt(), 6);
  }

  // Legacy leftover decision wrappers

  void testSetLeftoverDecisionPreservesBudgetLimit() {
    Category cat("Test", 100.0);

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
    Category cat("Test", 100.0);

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
    Category cat("Test", 100.0);

    // Set only leftover data (no budget limit)
    cat.setLeftoverDecision(2025, 6, { 50.0, 25.0 });

    // Clear leftover — entry should be removed entirely
    cat.clearLeftoverDecision(2025, 6);
    QVERIFY(cat.allMonthHistory().isEmpty());
  }

  // Budget limit for month (the core algorithm)

  void testBudgetLimitForMonthNoHistory() {
    Category cat("Groceries", -300.0);

    // No history at all — should return current budget limit
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 1, 1)), -300.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 6, 15)), -300.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2026, 12, 1)), -300.0);
  }

  void testBudgetLimitForMonthWithSingleHistoryEntry() {
    // Scenario: Budget was 250 until June, then changed to 300 in July.
    // When the change was made, the old limit (250) is recorded for June.
    Category cat("Groceries", -300.0);
    cat.setBudgetLimitForMonth(2025, 6, -250.0);  // June was the last month at 250

    // Months at or before June should see 250 (the old limit)
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 1, 1)), -250.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 6, 15)), -250.0);

    // July and after should see the current limit (300)
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 7, 1)), -300.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 12, 1)), -300.0);
  }

  void testBudgetLimitForMonthWithMultipleHistoryEntries() {
    // Scenario: Budget was 200 until March, changed to 250 in April,
    // then changed to 300 in July.
    Category cat("Groceries", -300.0);
    cat.setBudgetLimitForMonth(2025, 3, -200.0);  // March was the last month at 200
    cat.setBudgetLimitForMonth(2025, 6, -250.0);  // June was the last month at 250

    // Jan-March should see 200
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 1, 1)), -200.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 3, 1)), -200.0);

    // April-June should see 250
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 4, 1)), -250.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 6, 1)), -250.0);

    // July+ should see 300 (current)
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 7, 1)), -300.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 12, 1)), -300.0);
  }

  void testBudgetLimitForMonthIgnoresLeftoverOnlyEntries() {
    // History entries without budgetLimit should be skipped
    Category cat("Groceries", -300.0);
    cat.setBudgetLimitForMonth(2025, 3, -200.0);

    // Add a leftover-only entry in April (no budgetLimit)
    cat.setLeftoverDecision(2025, 4, { 50.0, 25.0 });

    // The leftover entry at April should not affect the budget limit lookup.
    // March has budgetLimit = 200. April has no budgetLimit.
    // For Jan-March, walk forward finds March's budgetLimit = 200.
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 1, 1)), -200.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 3, 1)), -200.0);

    // April: walk forward from April, find no budgetLimit at April, no more entries → current limit
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 4, 1)), -300.0);
  }

  void testBudgetLimitForMonthCrossYearBoundary() {
    Category cat("Groceries", -400.0);
    cat.setBudgetLimitForMonth(2024, 11, -300.0);  // Nov 2024 was the last month at 300

    // Before and at Nov 2024
    QCOMPARE(cat.budgetLimitForMonth(QDate(2024, 6, 1)), -300.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2024, 11, 1)), -300.0);

    // December 2024 and beyond
    QCOMPARE(cat.budgetLimitForMonth(QDate(2024, 12, 1)), -400.0);
    QCOMPARE(cat.budgetLimitForMonth(QDate(2025, 1, 1)), -400.0);
  }

  // setBudgetLimitForMonth / clearBudgetLimitForMonth

  void testSetBudgetLimitForMonth() {
    Category cat("Test", 100.0);

    QSignalSpy spy(&cat, &Category::monthHistoryChanged);
    cat.setBudgetLimitForMonth(2025, 6, 200.0);

    QCOMPARE(spy.count(), 1);
    MonthRecord record = cat.monthRecord(2025, 6);
    QVERIFY(record.budgetLimit.has_value());
    QCOMPARE(record.budgetLimit.value(), 200.0);
  }

  void testClearBudgetLimitForMonth() {
    Category cat("Test", 100.0);
    cat.setBudgetLimitForMonth(2025, 6, 200.0);

    QSignalSpy spy(&cat, &Category::monthHistoryChanged);
    cat.clearBudgetLimitForMonth(2025, 6);

    QCOMPARE(spy.count(), 1);
    // Entry should be removed since it had no leftover data
    QVERIFY(cat.allMonthHistory().isEmpty());
  }

  void testClearBudgetLimitForMonthPreservesLeftoverData() {
    Category cat("Test", 100.0);

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
    Category cat("Test", -200.0);

    cat.setLeftoverDecision(2025, 1, { 0.0, 30.0 });   // report 30
    cat.setLeftoverDecision(2025, 2, { 50.0, 20.0 });  // report 20
    cat.setLeftoverDecision(2025, 3, { 0.0, 10.0 });   // report 10

    // Before January: nothing
    QCOMPARE(cat.accumulatedLeftoverBefore(QDate(2025, 1, 1)), 0.0);

    // Before February: only January's report
    QCOMPARE(cat.accumulatedLeftoverBefore(QDate(2025, 2, 1)), 30.0);

    // Before March: January + February reports
    QCOMPARE(cat.accumulatedLeftoverBefore(QDate(2025, 3, 1)), 50.0);

    // Before April: all three
    QCOMPARE(cat.accumulatedLeftoverBefore(QDate(2025, 4, 1)), 60.0);
  }

  void testAccumulatedLeftoverBeforeIgnoresSaveAmounts() {
    Category cat("Test", -200.0);

    // Only save, no report
    cat.setLeftoverDecision(2025, 1, { 100.0, 0.0 });

    QCOMPARE(cat.accumulatedLeftoverBefore(QDate(2025, 2, 1)), 0.0);
  }

  void testAccumulatedLeftoverBeforeCrossYear() {
    Category cat("Test", -200.0);

    cat.setLeftoverDecision(2024, 11, { 0.0, 40.0 });
    cat.setLeftoverDecision(2024, 12, { 0.0, 25.0 });
    cat.setLeftoverDecision(2025, 1, { 0.0, 15.0 });

    // Before Feb 2025: all three
    QCOMPARE(cat.accumulatedLeftoverBefore(QDate(2025, 2, 1)), 80.0);

    // Before Jan 2025: only 2024 entries
    QCOMPARE(cat.accumulatedLeftoverBefore(QDate(2025, 1, 1)), 65.0);
  }

  // allMonthHistory

  void testAllMonthHistory() {
    Category cat("Test", 100.0);

    cat.setMonthRecord(2025, 1, { 10.0, 5.0 });
    cat.setMonthRecord(2025, 3, { 20.0, 10.0 });
    cat.setBudgetLimitForMonth(2025, 6, 200.0);

    auto history = cat.allMonthHistory();
    QCOMPARE(history.size(), 3);
    QVERIFY(history.contains({ 2025, 1 }));
    QVERIFY(history.contains({ 2025, 3 }));
    QVERIFY(history.contains({ 2025, 6 }));
  }
};

QTEST_GUILESS_MAIN(CategoryTest)
#include "CategoryTest.moc"
