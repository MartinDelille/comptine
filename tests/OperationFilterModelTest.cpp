#include <QTest>

#include "model/Account.h"
#include "model/OperationFilterModel.h"

class OperationFilterModelTest : public QObject {
  Q_OBJECT

private slots:
  void filtersTextAndAmount() {
    Account account("Fictional Checking", nullptr);
    auto* detailsMatch = account.addOperation(
        new Operation(&account, QDate(2026, 2, 3), -12.50, "Groceries", "Weekly shop"));
    account.addOperation(
        new Operation(&account, QDate(2026, 2, 2), 25.00, "Salary", "Monthly income"));
    account.addOperation(
        new Operation(&account, QDate(2026, 2, 2), 12.50, "Refund", "Returned purchase"));
    auto* amountMatch = account.addOperation(
        new Operation(&account, QDate(2026, 2, 1), -12.50, "Transport", "Bus pass"));

    OperationFilterModel model;
    model.setAccount(&account);

    QCOMPARE(model.rowCount(), 4);
    model.setQuery("weekly");
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.operationAt(0), detailsMatch);

    model.setQuery("12.50");
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.operationAt(0), detailsMatch);

    model.setQuery("12");
    QCOMPARE(model.rowCount(), 3);

    model.setQuery("-12");
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.operationAt(0), detailsMatch);
    QCOMPARE(model.operationAt(1), amountMatch);

    model.setQuery("+12");
    QCOMPARE(model.rowCount(), 1);

    model.setQuery("2.50");
    QCOMPARE(model.rowCount(), 0);

    model.setQuery("salary");
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.operationAt(0)->property("label").toString(), QString("Salary"));
  }

  void mapsSelectionAndNavigationToAccount() {
    Account account("Fictional Checking", nullptr);
    auto* newest = account.addOperation(
        new Operation(&account, QDate(2026, 3, 3), -10.0, "Newest"));
    auto* middle = account.addOperation(
        new Operation(&account, QDate(2026, 3, 2), -20.0, "Target"));
    auto* oldest = account.addOperation(
        new Operation(&account, QDate(2026, 3, 1), -30.0, "Oldest"));

    OperationFilterModel model;
    model.setAccount(&account);
    model.setQuery("target");
    model.selectAt(0);

    QCOMPARE(account.currentOperation(), middle);
    QVERIFY(account.isSelected(middle));
    model.nextOperation();
    QCOMPARE(account.currentOperation(), middle);

    model.setQuery("newest");
    QCOMPARE(model.currentOperationIndex(), -1);
    QCOMPARE(account.currentOperation(), nullptr);
    QVERIFY(account.isSelected(middle));

    model.setQuery("");
    model.selectAt(0);
    QCOMPARE(account.currentOperation(), newest);
    model.selectAt(2, true);
    QCOMPARE(account.selectionCount(), 3);
    QVERIFY(account.isSelected(oldest));

    model.selectAt(0);
    model.nextOperation(true);
    model.nextOperation(true);
    model.previousOperation(true);
    QCOMPARE(account.currentOperation(), middle);
    QCOMPARE(account.selectionCount(), 2);
    QVERIFY(account.isSelected(newest));
    QVERIFY(account.isSelected(middle));
    QVERIFY(!account.isSelected(oldest));

    model.selectAt(1);
    model.moveOperation(-2);
    QCOMPARE(account.currentOperation(), newest);
    model.moveOperation(2);
    QCOMPARE(account.currentOperation(), oldest);
    model.moveOperation(-10, true);
    QCOMPARE(account.currentOperation(), newest);
    QCOMPARE(account.selectionCount(), 3);
    QVERIFY(account.isSelected(oldest));
    QVERIFY(account.isSelected(middle));
    QVERIFY(account.isSelected(newest));
  }

  void handlesAccountAndQueryLifecycle() {
    OperationFilterModel model;

    QCOMPARE(model.account(), nullptr);
    QCOMPARE(model.currentOperationIndex(), -1);
    QCOMPARE(model.operationAt(-1), nullptr);
    QCOMPARE(model.operationAt(0), nullptr);
    model.selectAt(-1);
    model.toggleSelectionAt(0);
    model.selectRange(0, 1);
    model.previousOperation();
    model.nextOperation();
    model.moveOperation(1);

    Account account("Fictional Lifecycle", nullptr);
    auto* operation = account.addOperation(
        new Operation(&account, QDate(2026, 4, 1), -42.0, "Original"));
    model.setAccount(&account);
    QCOMPARE(model.account(), &account);
    QCOMPARE(model.rowCount(), 1);
    model.setAccount(&account);
    model.setQuery("   ");
    QCOMPARE(model.rowCount(), 1);
    model.setQuery("original");
    QCOMPARE(model.operationAt(0), operation);
    model.setQuery("does not match");
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.currentOperationIndex(), -1);
    model.setQuery("does not match");
    model.setAccount(nullptr);
    QCOMPARE(model.account(), nullptr);
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.currentOperationIndex(), -1);
  }

  void filtersSignedAndLocalizedAmountForms() {
    Account account("Fictional Amounts", nullptr);
    auto* expense = account.addOperation(
        new Operation(&account, QDate(2026, 5, 2), -1234.5, "Expense"));
    auto* income = account.addOperation(
        new Operation(&account, QDate(2026, 5, 1), 1234.5, "Income"));
    OperationFilterModel model;
    model.setAccount(&account);

    model.setQuery("-1234.50");
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.operationAt(0), expense);
    model.setQuery("+1234.50");
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.operationAt(0), income);
    model.setQuery("+1234");
    QCOMPARE(model.rowCount(), 1);
    model.setQuery("-1234");
    QCOMPARE(model.rowCount(), 1);
    model.setQuery("+1234.50");
    QCOMPARE(model.rowCount(), 1);
    model.setQuery("-1234.50");
    QCOMPARE(model.rowCount(), 1);
    model.setQuery("+999");
    QCOMPARE(model.rowCount(), 0);
    model.setQuery("-999");
    QCOMPARE(model.rowCount(), 0);
  }

  void handlesNavigationAndSelectionEdges() {
    Account account("Fictional Edges", nullptr);
    auto* first = account.addOperation(
        new Operation(&account, QDate(2026, 6, 3), -3.0, "First"));
    auto* second = account.addOperation(
        new Operation(&account, QDate(2026, 6, 2), -2.0, "Second"));
    auto* third = account.addOperation(
        new Operation(&account, QDate(2026, 6, 1), -1.0, "Third"));
    OperationFilterModel model;
    model.setAccount(&account);

    model.previousOperation();
    QCOMPARE(account.currentOperation(), third);
    model.nextOperation();
    QVERIFY(account.currentOperation() != nullptr);
    model.selectRange(100, -100);
    QCOMPARE(account.selectionCount(), 3);
    model.selectAt(0);
    model.setQuery("third");
    model.selectAt(0, true);
    QCOMPARE(account.currentOperation(), third);
    QCOMPARE(account.selectionCount(), 1);
    model.setQuery("");
    model.selectAt(1);
    model.moveOperation(0);
    QCOMPARE(account.currentOperation(), second);
    model.moveOperation(-100);
    QCOMPARE(account.currentOperation(), first);
    model.moveOperation(100);
    QCOMPARE(account.currentOperation(), third);
    model.nextOperation();
    QCOMPARE(account.currentOperation(), third);
    model.previousOperation();
    QCOMPARE(account.currentOperation(), second);

    account.removeOperation(second);
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.currentOperationIndex(), -1);
    model.selectRange(0, 1);
    model.toggleSelectionAt(-1);
    model.toggleSelectionAt(10);
    QCOMPARE(account.selectionCount(), 2);
  }

  void operationChangesRefreshFilterAndCurrentOperation() {
    Account account("Fictional Updates", nullptr);
    auto* operation = account.addOperation(
        new Operation(&account, QDate(2026, 7, 1), -7.0, "Before"));
    OperationFilterModel model;
    model.setAccount(&account);
    model.selectAt(0);
    QCOMPARE(model.currentOperationIndex(), 0);

    model.setQuery("after");
    QCOMPARE(model.rowCount(), 0);
    operation->set_label("After");
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.currentOperationIndex(), -1);

    operation->set_label("Hidden");
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(account.currentOperation(), nullptr);
    account.clear();
    QCOMPARE(model.rowCount(), 0);
  }
};

QTEST_GUILESS_MAIN(OperationFilterModelTest)
#include "OperationFilterModelTest.moc"
