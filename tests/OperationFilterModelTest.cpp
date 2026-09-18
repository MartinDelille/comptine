#include <QTest>

#include "model/Account.h"
#include "model/OperationFilterModel.h"

using namespace Qt::StringLiterals;

class OperationFilterModelTest : public QObject {
  Q_OBJECT

private slots:
  void filtersTextAndAmount() {
    Account account(u"Fictional Checking"_s, nullptr);
    auto* detailsMatch = account.addOperation(
        new Operation(&account, QDate(2026, 2, 3), -12.50, u"Groceries"_s, {}, u"Weekly shop"_s));
    account.addOperation(
        new Operation(&account, QDate(2026, 2, 2), 25.00, u"Salary"_s, {}, u"Monthly income"_s));
    account.addOperation(
        new Operation(&account, QDate(2026, 2, 2), 12.50, u"Refund"_s, {}, u"Returned purchase"_s));
    auto* amountMatch = account.addOperation(
        new Operation(&account, QDate(2026, 2, 1), -12.50, u"Transport"_s, {}, u"Bus pass"_s));

    OperationFilterModel model;
    model.setAccount(&account);

    QCOMPARE(model.rowCount(), 4);
    model.setQuery(u"weekly"_s);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.operationAt(0), detailsMatch);

    model.setQuery(u"12.50"_s);
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.operationAt(0), detailsMatch);

    model.setQuery(u"12"_s);
    QCOMPARE(model.rowCount(), 3);

    model.setQuery(u"-12"_s);
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.operationAt(0), detailsMatch);
    QCOMPARE(model.operationAt(1), amountMatch);

    model.setQuery(u"+12"_s);
    QCOMPARE(model.rowCount(), 1);

    model.setQuery(u"2.50"_s);
    QCOMPARE(model.rowCount(), 0);

    model.setQuery(u"salary"_s);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.operationAt(0)->property("label").toString(), u"Salary"_s);
  }

  void mapsSelectionAndNavigationToAccount() {
    Account account(u"Fictional Checking"_s, nullptr);
    auto* newest = account.addOperation(
        new Operation(&account, QDate(2026, 3, 3), -10.0, u"Newest"_s));
    auto* middle = account.addOperation(
        new Operation(&account, QDate(2026, 3, 2), -20.0, u"Target"_s));
    auto* oldest = account.addOperation(
        new Operation(&account, QDate(2026, 3, 1), -30.0, u"Oldest"_s));

    OperationFilterModel model;
    model.setAccount(&account);
    model.setQuery(u"target"_s);
    model.selectAt(0);

    QCOMPARE(account.currentOperation(), middle);
    QVERIFY(account.isSelected(middle));
    model.nextOperation();
    QCOMPARE(account.currentOperation(), middle);

    model.setQuery(u"newest"_s);
    QCOMPARE(model.currentOperationIndex(), -1);
    QCOMPARE(account.currentOperation(), nullptr);
    QVERIFY(account.isSelected(middle));

    model.setQuery(""_L1);
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

    Account account(u"Fictional Lifecycle"_s, nullptr);
    auto* operation = account.addOperation(
        new Operation(&account, QDate(2026, 4, 1), -42.0, u"Original"_s));
    model.setAccount(&account);
    QCOMPARE(model.account(), &account);
    QCOMPARE(model.rowCount(), 1);
    model.setAccount(&account);
    model.setQuery(u"   "_s);
    QCOMPARE(model.rowCount(), 1);
    model.setQuery(u"original"_s);
    QCOMPARE(model.operationAt(0), operation);
    model.setQuery(u"does not match"_s);
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.currentOperationIndex(), -1);
    model.setQuery(u"does not match"_s);
    model.setAccount(nullptr);
    QCOMPARE(model.account(), nullptr);
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.currentOperationIndex(), -1);
  }

  void filtersSignedAndLocalizedAmountForms() {
    Account account(u"Fictional Amounts"_s, nullptr);
    auto* expense = account.addOperation(
        new Operation(&account, QDate(2026, 5, 2), -1234.5, u"Expense"_s));
    auto* income = account.addOperation(
        new Operation(&account, QDate(2026, 5, 1), 1234.5, u"Income"_s));
    OperationFilterModel model;
    model.setAccount(&account);

    model.setQuery(u"-1234.50"_s);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.operationAt(0), expense);
    model.setQuery(u"+1234.50"_s);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.operationAt(0), income);
    model.setQuery(u"+1234"_s);
    QCOMPARE(model.rowCount(), 1);
    model.setQuery(u"-1234"_s);
    QCOMPARE(model.rowCount(), 1);
    model.setQuery(u"+1234.50"_s);
    QCOMPARE(model.rowCount(), 1);
    model.setQuery(u"-1234.50"_s);
    QCOMPARE(model.rowCount(), 1);
    model.setQuery(u"+999"_s);
    QCOMPARE(model.rowCount(), 0);
    model.setQuery(u"-999"_s);
    QCOMPARE(model.rowCount(), 0);
  }

  void handlesNavigationAndSelectionEdges() {
    Account account(u"Fictional Edges"_s, nullptr);
    auto* first = account.addOperation(
        new Operation(&account, QDate(2026, 6, 3), -3.0, u"First"_s));
    auto* second = account.addOperation(
        new Operation(&account, QDate(2026, 6, 2), -2.0, u"Second"_s));
    auto* third = account.addOperation(
        new Operation(&account, QDate(2026, 6, 1), -1.0, u"Third"_s));
    OperationFilterModel model;
    model.setAccount(&account);

    model.previousOperation();
    QCOMPARE(account.currentOperation(), third);
    model.nextOperation();
    QVERIFY(account.currentOperation() != nullptr);
    model.selectRange(100, -100);
    QCOMPARE(account.selectionCount(), 3);
    model.selectAt(0);
    model.setQuery(u"third"_s);
    model.selectAt(0, true);
    QCOMPARE(account.currentOperation(), third);
    QCOMPARE(account.selectionCount(), 1);
    model.setQuery(""_L1);
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
    Account account(u"Fictional Updates"_s, nullptr);
    auto* operation = account.addOperation(
        new Operation(&account, QDate(2026, 7, 1), -7.0, u"Before"_s));
    OperationFilterModel model;
    model.setAccount(&account);
    model.selectAt(0);
    QCOMPARE(model.currentOperationIndex(), 0);

    model.setQuery(u"after"_s);
    QCOMPARE(model.rowCount(), 0);
    operation->set_label(u"After"_s);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.currentOperationIndex(), -1);

    operation->set_label(u"Hidden"_s);
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(account.currentOperation(), nullptr);
    account.clear();
    QCOMPARE(model.rowCount(), 0);
  }
};

QTEST_GUILESS_MAIN(OperationFilterModelTest)
#include "OperationFilterModelTest.moc"
