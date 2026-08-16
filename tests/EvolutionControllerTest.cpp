#include <QDate>
#include <QTest>

#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "services/BudgetData.h"
#include "services/CategoryController.h"
#include "services/EvolutionController.h"

class EvolutionControllerTest : public QObject {
  Q_OBJECT

  static void addOperation(BudgetData& budgetData, const QDate& date) {
    auto* account = budgetData.accounts().isEmpty()
                        ? budgetData.createAccount("Fictional Account")
                        : budgetData.accounts().first();
    account->addOperation(new Operation(account, date, -10.0, "Fictional purchase"));
  }

  static void addOperation(BudgetData& budgetData, const QDate& date,
                           const QDate& budgetDate) {
    auto* account = budgetData.accounts().isEmpty()
                        ? budgetData.createAccount("Fictional Account")
                        : budgetData.accounts().first();
    auto* operation = new Operation(account, date, -10.0, "Fictional purchase");
    operation->set_budgetDate(budgetDate);
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
    category->setMonthRecord(2025, 1, { 1.0, 0.0 });
    category->setMonthRecord(2025, 5, { 2.0, 0.0 });
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
    category->setMonthRecord(2025, 3, { 1.0, 0.0 });
    category->setMonthRecord(2025, 5, { 2.0, 0.0 });
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

    addOperation(budgetData, QDate(2025, 1, 10), QDate(2024, 12, 1));

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
    category->setBudgetLimitForMonth(2024, 1, -75.0);
    EvolutionController evolution(budgetData, categories);

    QCOMPARE(evolution.monthCount(), 18);
    const QDate expectedHeader(2024, 1, 1);
    QCOMPARE(evolution.headerData(0, Qt::Horizontal).toDate(), expectedHeader);
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
  }

  void metricChangesCellValue() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    const QDate month(2025, 3, 1);
    budgetData.set_budgetDate(month);
    CategoryController categories(budgetData, undoStack);
    auto* category = categories.addCategory(new Category("Fictional Metrics", -100.0));
    MonthRecord record;
    record.saveAmount = 20.0;
    record.reportAmount = -5.0;
    category->setMonthRecord(2025, 3, record);
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
};

QTEST_GUILESS_MAIN(EvolutionControllerTest)
#include "EvolutionControllerTest.moc"
