#include <QDate>
#include <QSignalSpy>
#include <QTest>

#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "services/BudgetData.h"
#include "services/CategoryController.h"
#include "services/EvolutionController.h"

class EvolutionControllerTest : public QObject {
  Q_OBJECT

  struct OperationDates {
    QDate operation;
    QDate budget;
  };

  static void addOperation(BudgetData& budgetData, const QDate& date) {
    auto* account = budgetData.accounts().isEmpty()
                        ? budgetData.createAccount("Fictional Account")
                        : budgetData.accounts().first();
    account->addOperation(new Operation(account, date, -10.0, "Fictional purchase"));
  }

  static void addOperation(BudgetData& budgetData, const OperationDates& dates) {
    auto* account = budgetData.accounts().isEmpty()
                        ? budgetData.createAccount("Fictional Account")
                        : budgetData.accounts().first();
    auto* operation = new Operation(account, dates.operation, -10.0, "Fictional purchase");
    operation->set_budgetDate(dates.budget);
    account->addOperation(operation);
  }

private slots:
  void noHistoryUsesSelectedMonth() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    budgetData.set_budgetDate(QDate(2025, 6, 15));
    CategoryController categories(budgetData, undoStack);
    EvolutionController evolution(budgetData, categories);

    QCOMPARE(evolution.monthCount(), 1);
    QCOMPARE(evolution.columnCount(), 1);
    const QDate expectedHeader(2025, 6, 1);
    QCOMPARE(evolution.headerData(0, Qt::Horizontal).toDate(), expectedHeader);
    QCOMPARE(evolution.headerData(0, Qt::Horizontal, EvolutionController::MonthDateRole).toDate(), expectedHeader);
    QVERIFY(!evolution.headerData(0, Qt::Vertical, EvolutionController::MonthDateRole).isValid());
    QCOMPARE(evolution.currentMonthIndex(), 0);
    QCOMPARE(evolution.firstMonth(), expectedHeader);
    QCOMPARE(evolution.lastMonth(), expectedHeader);
  }

  void historyFillsGapsAndIncludesSelection() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    budgetData.set_budgetDate(QDate(2025, 3, 20));
    CategoryController categories(budgetData, undoStack);
    auto* category = categories.addCategory(new Category("Fictional Food"));
    category->setMonthRecord(QDate(2025, 1, 1), { 1.0, 0.0 });
    category->setMonthRecord(QDate(2025, 5, 1), { 2.0, 0.0 });
    addOperation(budgetData, QDate(2025, 1, 10));
    addOperation(budgetData, QDate(2025, 5, 10));
    EvolutionController evolution(budgetData, categories);

    QCOMPARE(evolution.monthCount(), 5);
    QCOMPARE(evolution.columnCount(), 5);
    const QDate expectedHeader(2025, 3, 1);
    QCOMPARE(evolution.headerData(2, Qt::Horizontal).toDate(), expectedHeader);
    QCOMPARE(evolution.currentMonthIndex(), 2);
    QCOMPARE(evolution.firstMonth(), QDate(2025, 1, 1));
    QCOMPARE(evolution.lastMonth(), QDate(2025, 5, 1));
  }

  void selectionOutsideHistoryIsAnEndpoint() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    CategoryController categories(budgetData, undoStack);
    auto* category = categories.addCategory(new Category("Fictional Transit"));
    category->setMonthRecord(QDate(2025, 3, 1), { 1.0, 0.0 });
    category->setMonthRecord(QDate(2025, 5, 1), { 2.0, 0.0 });
    addOperation(budgetData, QDate(2025, 3, 10));
    addOperation(budgetData, QDate(2025, 5, 10));
    EvolutionController evolution(budgetData, categories);

    budgetData.set_budgetDate(QDate(2025, 1, 1));
    const QDate expectedFirstHeader(2025, 1, 1);
    QCOMPARE(evolution.headerData(0, Qt::Horizontal).toDate(), expectedFirstHeader);
    QCOMPARE(evolution.currentMonthIndex(), 0);

    budgetData.set_budgetDate(QDate(2025, 7, 1));
    const QDate expectedLastHeader(2025, 7, 1);
    QCOMPARE(evolution.headerData(evolution.columnCount() - 1, Qt::Horizontal).toDate(), expectedLastHeader);
    QCOMPARE(evolution.currentMonthIndex(), 4);
  }

  void budgetDateDefinesHistoryRange() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    budgetData.set_budgetDate(QDate(2025, 1, 1));
    CategoryController categories(budgetData, undoStack);
    EvolutionController evolution(budgetData, categories);

    addOperation(budgetData, { QDate(2025, 1, 10), QDate(2024, 12, 1) });

    QCOMPARE(evolution.monthCount(), 2);
    const QDate expectedHeader(2024, 12, 1);
    QCOMPARE(evolution.headerData(0, Qt::Horizontal).toDate(), expectedHeader);
  }

  void monthHistoryExtendsHistoryRange() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    budgetData.set_budgetDate(QDate(2025, 6, 1));
    CategoryController categories(budgetData, undoStack);
    auto* category = categories.addCategory(new Category("Fictional Utilities"));
    category->setBudgetLimitForMonth(QDate(2024, 1, 1), -75.0);
    EvolutionController evolution(budgetData, categories);

    QCOMPARE(evolution.monthCount(), 18);
    const QDate expectedHeader(2024, 1, 1);
    QCOMPARE(evolution.headerData(0, Qt::Horizontal).toDate(), expectedHeader);
  }

  void budgetLimitChangeKeepsEffectiveMonthInRange() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    budgetData.set_budgetDate(QDate(2025, 6, 1));
    CategoryController categories(budgetData, undoStack);
    auto* category = categories.addCategory(new Category("Fictional Utilities"));
    category->setBudgetLimitForMonth(QDate(2025, 6, 1), -100.0);
    EvolutionController evolution(budgetData, categories);

    QCOMPARE(evolution.monthCount(), 1);
    QCOMPARE(evolution.lastMonth(), QDate(2025, 6, 1));

    budgetData.set_budgetDate(QDate(2025, 5, 1));

    QCOMPARE(evolution.monthCount(), 2);
    QCOMPARE(evolution.lastMonth(), QDate(2025, 6, 1));
  }

  void operationChangesUpdateHistoryRange() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    budgetData.set_budgetDate(QDate(2025, 6, 1));
    CategoryController categories(budgetData, undoStack);
    EvolutionController evolution(budgetData, categories);

    addOperation(budgetData, QDate(2023, 1, 10));

    QCOMPARE(evolution.monthCount(), 30);
    const QDate expectedHeader(2023, 1, 1);
    QCOMPARE(evolution.headerData(0, Qt::Horizontal).toDate(), expectedHeader);
    QCOMPARE(evolution.summaryStartIndex(), 0);
    QCOMPARE(evolution.summaryEndIndex(), 29);
  }

  void metricChangesCellValue() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    const QDate month(2025, 3, 1);
    budgetData.set_budgetDate(month);
    CategoryController categories(budgetData, undoStack);
    auto* category = categories.addCategory(new Category("Fictional Metrics"));
    MonthRecord record;
    record.saveAmount = 20.0;
    record.reportAmount = -5.0;
    record.budgetLimit = -100.0;
    category->setMonthRecord(QDate(2025, 3, 1), record);
    auto* account = budgetData.createAccount("Fictional Account");
    auto* operation = new Operation(account, QDate(2025, 3, 10), -30.0);
    operation->setAllocations({ new Allocation(category, -30.0) });
    account->addOperation(operation);
    EvolutionController evolution(budgetData, categories);
    const QModelIndex cell = evolution.index(0, 0);

    categories.set_currentIndex(0);
    QVERIFY(evolution.data(cell, EvolutionController::CurrentMonthRole).toBool());
    QVERIFY(evolution.data(cell, EvolutionController::CurrentCategoryRole).toBool());
    QVERIFY(evolution.headerData(0, Qt::Horizontal, EvolutionController::CurrentMonthRole).toBool());
    QVERIFY(evolution.headerData(0, Qt::Vertical, EvolutionController::CurrentCategoryRole).toBool());
    QCOMPARE(evolution.data(cell, EvolutionController::BudgetRole).toDouble(), -100.0);
    QCOMPARE(evolution.data(cell, EvolutionController::SpentRole).toDouble(), -30.0);
    QCOMPARE(evolution.data(cell, EvolutionController::LeftoverRole).toDouble(), 70.0);
    QCOMPARE(evolution.data(cell, EvolutionController::SavedRole).toDouble(), 20.0);
    QCOMPARE(evolution.data(cell, EvolutionController::ReportedRole).toDouble(), -5.0);
    QCOMPARE(evolution.data(cell, EvolutionController::AccumulatedRole).toDouble(), -5.0);
  }

  void exposesBudgetLimitChangeRole() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    budgetData.set_budgetDate(QDate(2025, 6, 1));
    CategoryController categories(budgetData, undoStack);
    auto* category = categories.addCategory(new Category("Fictional Boundaries"));
    category->setBudgetLimitForMonth(QDate(2025, 1, 1), -200.0);
    category->setBudgetLimitForMonth(QDate(2025, 6, 1), -250.0);
    EvolutionController evolution(budgetData, categories);

    const QModelIndex january = evolution.index(0, 0);
    const QModelIndex june = evolution.index(0, 5);
    QVERIFY(evolution.data(january, EvolutionController::BudgetLimitChangeRole).toBool());
    QVERIFY(evolution.data(june, EvolutionController::BudgetLimitChangeRole).toBool());
    QCOMPARE(evolution.data(evolution.index(0, 1), EvolutionController::BudgetLimitChangeRole).toBool(), false);
    QCOMPARE(evolution.roleNames().value(EvolutionController::BudgetLimitChangeRole), QByteArray("budgetLimitChange"));
  }

  void fixedCategorySummariesUseTheirOwnMetrics() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    budgetData.set_budgetDate(QDate(2025, 2, 1));
    CategoryController categories(budgetData, undoStack);
    auto* category = categories.addCategory(new Category("Fictional Summary"));
    category->setMonthRecord(QDate(2025, 1, 1), { 0.0, -7.0 });
    category->setMonthRecord(QDate(2025, 2, 1), { 0.0, -9.0 });
    category->setBudgetLimitForMonth(QDate(2025, 1, 1), -200.0);
    category->setBudgetLimitForMonth(QDate(2025, 2, 1), -100.0);
    auto* account = budgetData.createAccount("Fictional Account");

    auto* firstOperation = new Operation(account, QDate(2025, 1, 10), -10.0);
    firstOperation->setAllocations({ new Allocation(category, -10.0) });
    account->addOperation(firstOperation);
    auto* secondOperation = new Operation(account, QDate(2025, 2, 10), -30.0);
    secondOperation->setAllocations({ new Allocation(category, -30.0) });
    account->addOperation(secondOperation);

    EvolutionController evolution(budgetData, categories);

    QCOMPARE(evolution.summaryStartIndex(), 0);
    QCOMPARE(evolution.summaryEndIndex(), 1);
    QCOMPARE(evolution.headerData(0, Qt::Vertical,
                                  EvolutionController::SpentAverageRole)
                 .toDouble(),
             -20.0);
    QCOMPARE(evolution.headerData(0, Qt::Vertical,
                                  EvolutionController::ReportedSumRole)
                 .toDouble(),
             -16.0);
    QCOMPARE(evolution.headerData(0, Qt::Horizontal,
                                  EvolutionController::MonthlySumRole)
                 .toDouble(),
             -200.0);
    QCOMPARE(evolution.headerData(1, Qt::Horizontal,
                                  EvolutionController::MonthlySumRole)
                 .toDouble(),
             -100.0);

    QCOMPARE(evolution.headerData(0, Qt::Vertical,
                                  EvolutionController::SpentAverageRole)
                 .toDouble(),
             -20.0);
    QCOMPARE(evolution.headerData(0, Qt::Vertical,
                                  EvolutionController::ReportedSumRole)
                 .toDouble(),
             -16.0);

    evolution.set_selectedMetric(1);
    QCOMPARE(evolution.headerData(0, Qt::Vertical,
                                  EvolutionController::SpentAverageRole)
                 .toDouble(),
             -20.0);
    QCOMPARE(evolution.headerData(0, Qt::Vertical,
                                  EvolutionController::ReportedSumRole)
                 .toDouble(),
             -16.0);
    QCOMPARE(evolution.headerData(0, Qt::Horizontal,
                                  EvolutionController::MonthlySumRole)
                 .toDouble(),
             -10.0);
    QCOMPARE(evolution.headerData(1, Qt::Horizontal,
                                  EvolutionController::MonthlySumRole)
                 .toDouble(),
             -30.0);
  }

  void spentAverageUsesSelectedRangeAndReportedSumUsesFullRange() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    budgetData.set_budgetDate(QDate(2025, 3, 1));
    CategoryController categories(budgetData, undoStack);
    auto* category = categories.addCategory(new Category("Fictional Range"));
    category->setBudgetLimitForMonth(QDate(2025, 1, 1), -10.0);
    category->setBudgetLimitForMonth(QDate(2025, 2, 1), -20.0);
    category->setBudgetLimitForMonth(QDate(2025, 3, 1), -30.0);
    category->setMonthRecord(QDate(2025, 1, 1), { 0.0, -1.0 });
    category->setMonthRecord(QDate(2025, 2, 1), { 0.0, -2.0 });
    category->setMonthRecord(QDate(2025, 3, 1), { 0.0, -3.0 });
    auto* account = budgetData.createAccount("Fictional Account");
    auto* firstOperation = new Operation(account, QDate(2025, 1, 10), -10.0);
    firstOperation->setAllocations({ new Allocation(category, -10.0) });
    account->addOperation(firstOperation);
    auto* lastOperation = new Operation(account, QDate(2025, 3, 10), -30.0);
    lastOperation->setAllocations({ new Allocation(category, -30.0) });
    account->addOperation(lastOperation);
    EvolutionController evolution(budgetData, categories);

    QCOMPARE(evolution.summaryStartIndex(), 0);
    QCOMPARE(evolution.summaryEndIndex(), 2);
    QCOMPARE(evolution.headerData(0, Qt::Vertical,
                                  EvolutionController::SpentAverageRole)
                 .toDouble(),
             -40.0 / 3.0);
    QCOMPARE(evolution.headerData(0, Qt::Vertical,
                                  EvolutionController::ReportedSumRole)
                 .toDouble(),
             -6.0);

    evolution.set_selectedMetric(1);
    QCOMPARE(evolution.headerData(0, Qt::Vertical,
                                  EvolutionController::SpentAverageRole)
                 .toDouble(),
             -40.0 / 3.0);
    QCOMPARE(evolution.headerData(0, Qt::Vertical,
                                  EvolutionController::ReportedSumRole)
                 .toDouble(),
             -6.0);
    evolution.set_selectedMetric(0);

    evolution.setSummaryStartMonthIndex(0);
    QCOMPARE(evolution.summaryStartIndex(), 0);
    QCOMPARE(evolution.summaryEndIndex(), 2);
    QCOMPARE(evolution.headerData(0, Qt::Vertical,
                                  EvolutionController::SpentAverageRole)
                 .toDouble(),
             -40.0 / 3.0);
    QCOMPARE(evolution.headerData(0, Qt::Vertical,
                                  EvolutionController::ReportedSumRole)
                 .toDouble(),
             -6.0);

    evolution.setSummaryStartMonthIndex(1);
    QCOMPARE(evolution.summaryStartIndex(), 1);
    QCOMPARE(evolution.summaryEndIndex(), 2);
    QCOMPARE(evolution.headerData(0, Qt::Vertical,
                                  EvolutionController::SpentAverageRole)
                 .toDouble(),
             -15.0);
    QCOMPARE(evolution.headerData(0, Qt::Vertical,
                                  EvolutionController::ReportedSumRole)
                 .toDouble(),
             -6.0);

    evolution.setSummaryEndMonthIndex(0);
    QCOMPARE(evolution.summaryStartIndex(), 0);
    QCOMPARE(evolution.summaryEndIndex(), 0);
    QCOMPARE(evolution.headerData(0, Qt::Vertical,
                                  EvolutionController::SpentAverageRole)
                 .toDouble(),
             -10.0);
    QCOMPARE(evolution.headerData(0, Qt::Vertical,
                                  EvolutionController::ReportedSumRole)
                 .toDouble(),
             -6.0);
  }

  void changingSelectedMonthOnlyRefreshesSelectionRole() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    budgetData.set_budgetDate(QDate(2025, 2, 1));
    CategoryController categories(budgetData, undoStack);
    categories.addCategory(new Category("Fictional Selection"));
    addOperation(budgetData, QDate(2025, 1, 10));
    addOperation(budgetData, QDate(2025, 3, 10));
    EvolutionController evolution(budgetData, categories);
    QSignalSpy dataChangedSpy(&evolution, &QAbstractItemModel::dataChanged);

    budgetData.set_budgetDate(QDate(2025, 1, 1));

    QCOMPARE(dataChangedSpy.count(), 1);
    const QList<int> roles = dataChangedSpy.at(0).at(2).value<QList<int>>();
    QCOMPARE(roles, QList<int>{ EvolutionController::CurrentMonthRole });
  }

  void modelHandlesEmptyAndInvalidIndexes() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    budgetData.set_budgetDate(QDate(2025, 6, 15));
    CategoryController categories(budgetData, undoStack);
    EvolutionController evolution(budgetData, categories);

    const QModelIndex invalid;
    const QModelIndex child = evolution.index(0, 0, evolution.index(0, 0));
    QVERIFY(!evolution.data(invalid).isValid());
    QVERIFY(!evolution.data(evolution.index(0, -1)).isValid());
    QVERIFY(!evolution.data(evolution.index(0, evolution.columnCount())).isValid());
    QCOMPARE(evolution.rowCount(invalid), 0);
    QCOMPARE(evolution.columnCount(invalid), 1);
    QCOMPARE(evolution.rowCount(evolution.index(0, 0)), 0);
    QVERIFY(!child.isValid());
    categories.addCategory(new Category("Fictional Parent"));
    const QModelIndex validParent = evolution.index(0, 0);
    QCOMPARE(evolution.rowCount(validParent), 0);
    QCOMPARE(evolution.columnCount(validParent), 0);
    QVERIFY(!evolution.headerData(-1, Qt::Horizontal).isValid());
    QVERIFY(!evolution.headerData(evolution.columnCount(), Qt::Horizontal).isValid());
    QVERIFY(evolution.headerData(0, Qt::Vertical).toString() == QString("Fictional Parent"));
    QVERIFY(!evolution.headerData(0, Qt::Horizontal, EvolutionController::BudgetRole).isValid());
  }

  void exposesLabelsAndAllMetricRoles() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    budgetData.set_budgetDate(QDate(2025, 3, 1));
    CategoryController categories(budgetData, undoStack);
    auto* category = categories.addCategory(new Category("Fictional Metrics"));
    category->setMonthRecord(QDate(2025, 3, 1), { 12.0, -4.0 });
    category->setBudgetLimitForMonth(QDate(2025, 3, 1), -100.0);
    auto* account = budgetData.createAccount("Fictional Account");
    auto* operation = new Operation(account, QDate(2025, 3, 10), -25.0);
    operation->setAllocations({ new Allocation(category, -25.0) });
    account->addOperation(operation);
    EvolutionController evolution(budgetData, categories);
    const QModelIndex cell = evolution.index(0, 0);

    QCOMPARE(evolution.availableMonthLabels().size(), 1);
    QVERIFY(!evolution.availableMonthLabels().first().isEmpty());
    QCOMPARE(evolution.data(cell, EvolutionController::DisplayRole).toDouble(), -100.0);
    QCOMPARE(evolution.data(cell, EvolutionController::CategoryNameRole).toString(), QString("Fictional Metrics"));
    QCOMPARE(evolution.data(cell, EvolutionController::MonthDateRole).toDate(), QDate(2025, 3, 1));
    QCOMPARE(evolution.data(cell, EvolutionController::SpentAverageRole).toDouble(), -25.0);
    QCOMPARE(evolution.data(cell, EvolutionController::ReportedSumRole).toDouble(), -4.0);
    QCOMPARE(evolution.data(cell, EvolutionController::MonthlySumRole).toDouble(), -100.0);
    evolution.set_selectedMetric(99);
    QCOMPARE(evolution.headerData(0, Qt::Horizontal, EvolutionController::MonthlySumRole).toDouble(), -100.0);
    QVERIFY(!evolution.data(cell, Qt::UserRole + 1000).isValid());
  }

  void summaryIndexesClampAndKeepRangeOrdered() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    budgetData.set_budgetDate(QDate(2025, 3, 1));
    CategoryController categories(budgetData, undoStack);
    auto* category = categories.addCategory(new Category("Fictional Summary"));
    category->setBudgetLimitForMonth(QDate(2025, 1, 1), -10.0);
    category->setBudgetLimitForMonth(QDate(2025, 3, 1), -30.0);
    addOperation(budgetData, QDate(2025, 1, 10));
    EvolutionController evolution(budgetData, categories);

    const QDate initialStart = evolution.summaryStartMonth();
    evolution.setSummaryStartMonthIndex(-1);
    evolution.setSummaryEndMonthIndex(evolution.monthCount());
    QCOMPARE(evolution.summaryStartMonth(), initialStart);
    QCOMPARE(evolution.summaryEndMonth(), QDate(2025, 1, 1));
    evolution.setSummaryEndMonthIndex(evolution.monthCount() - 1);
    QCOMPARE(evolution.summaryEndMonth(), QDate(2025, 3, 1));

    evolution.set_summaryStartMonth(QDate(2025, 3, 20));
    QCOMPARE(evolution.summaryStartMonth(), QDate(2025, 3, 1));
    QCOMPARE(evolution.summaryEndMonth(), QDate(2025, 3, 1));
    evolution.set_summaryEndMonth(QDate(2024, 1, 1));
    QCOMPARE(evolution.summaryStartMonth(), QDate(2025, 1, 1));
    QCOMPARE(evolution.summaryEndMonth(), QDate(2025, 1, 1));
  }

  void refreshSignalsFollowCategoryChanges() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    budgetData.set_budgetDate(QDate(2025, 2, 1));
    CategoryController categories(budgetData, undoStack);
    EvolutionController evolution(budgetData, categories);
    QSignalSpy resetSpy(&evolution, &QAbstractItemModel::modelReset);
    QSignalSpy dataChangedSpy(&evolution, &QAbstractItemModel::dataChanged);
    QSignalSpy horizontalHeaderSpy(&evolution, &QAbstractItemModel::headerDataChanged);

    categories.addCategory(new Category("Fictional Added"));
    QVERIFY(resetSpy.count() >= 1);

    resetSpy.clear();
    dataChangedSpy.clear();
    categories.at(0)->setMonthRecord(QDate(2025, 2, 1), { 1.0, -2.0 });
    QVERIFY(dataChangedSpy.count() >= 1);
    QVERIFY(horizontalHeaderSpy.count() >= 1);

    categories.set_currentIndex(0);
    QVERIFY(dataChangedSpy.count() >= 2);
  }
};

QTEST_GUILESS_MAIN(EvolutionControllerTest)
#include "EvolutionControllerTest.moc"
