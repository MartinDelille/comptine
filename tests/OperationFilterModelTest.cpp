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
};

QTEST_GUILESS_MAIN(OperationFilterModelTest)
#include "OperationFilterModelTest.moc"
