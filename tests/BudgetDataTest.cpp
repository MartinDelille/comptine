#include <QSignalSpy>
#include <QTest>
#include <QUndoStack>
#include <QUrl>

#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "services/BudgetData.h"

using namespace Qt::StringLiterals;

class BudgetDataTest : public QObject {
  Q_OBJECT

private slots:
  void navigatesMonthsAndAccounts() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    budgetData.set_budgetDate(QDate(2026, 3, 15));

    budgetData.previousMonth();
    QCOMPARE(budgetData.budgetDate(), QDate(2026, 2, 1));
    budgetData.nextMonth();
    QCOMPARE(budgetData.budgetDate(), QDate(2026, 3, 1));

    auto* first = budgetData.createAccount(u"Fictional Checking"_s);
    auto* second = budgetData.createAccount(u"Fictional Savings"_s);
    QCOMPARE(budgetData.rowCount(), 2);
    QCOMPARE(budgetData.currentAccountIndex(), -1);
    budgetData.set_currentAccountIndex(1);
    QCOMPARE(budgetData.currentAccount(), second);
    QCOMPARE(budgetData.currentAccountIndex(), 1);

    QVERIFY(budgetData.at(-1) == nullptr);
    QVERIFY(budgetData.at(2) == nullptr);
    QCOMPARE(budgetData.accountByName("Fictional Checking"_L1), first);
    QVERIFY(budgetData.accountByName("Missing"_L1) == nullptr);
    QCOMPARE(budgetData.accountIndex(first), 0);
    QCOMPARE(budgetData.accountIndex(nullptr), -1);

    const auto roles = budgetData.roleNames();
    QCOMPARE(roles.value(BudgetData::NameRole), "name"_ba);
    QCOMPARE(roles.value(BudgetData::OperationCountRole), "operationCount"_ba);
    QCOMPARE(roles.value(BudgetData::AccountRole), "account"_ba);
    QVERIFY(!budgetData.data(QModelIndex(), BudgetData::NameRole).isValid());
    QVERIFY(!budgetData.data(budgetData.index(5, 0), BudgetData::NameRole).isValid());
    QVERIFY(!budgetData.data(budgetData.index(0, 0), Qt::DisplayRole).isValid());
    QCOMPARE(budgetData.data(budgetData.index(0, 0), BudgetData::NameRole).toString(),
             u"Fictional Checking"_s);
    QCOMPARE(budgetData.data(budgetData.index(0, 0), BudgetData::OperationCountRole).toInt(), 0);
    QCOMPARE(budgetData.data(budgetData.index(0, 0), BudgetData::AccountRole).value<Account*>(),
             first);
    QCOMPARE(budgetData.rowCount(budgetData.index(0, 0)), 0);
  }

  void matchesAccountsToImportUrls() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* account = budgetData.createAccount(u"Fictional Bank"_s);
    account->addImportSourcePrefix(u"fictional-bank"_s);
    budgetData.createAccount(u"Fictional Card"_s);

    QCOMPARE(budgetData.suggestedAccountForUrl(QUrl::fromLocalFile("/tmp/Fictional Bank.csv"_L1)),
             u"Fictional Bank"_s);
    QCOMPARE(budgetData.suggestedAccountForUrl(QUrl::fromLocalFile("/tmp/fictional-bank-2026.csv"_L1)),
             u"Fictional Bank"_s);
    QCOMPARE(budgetData.suggestedAccountForUrl(QUrl::fromLocalFile("/tmp/unmatched.csv"_L1)),
             u"unmatched"_s);
  }

  void navigatesOperationsAndCountsCategories() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    Category category(u"Fictional Category"_s);
    Category otherCategory(u"Fictional Other"_s);
    auto* account = budgetData.createAccount(u"Fictional Checking"_s);
    auto* otherAccount = budgetData.createAccount(u"Fictional Savings"_s);
    auto* operation = account->addOperation(
        new Operation(account, QDate(2026, 1, 1), -12.0, u"Fictional Purchase"_s,
                      { new Allocation(&category, -12.0) }, {}));
    otherAccount->addOperation(
        new Operation(otherAccount, QDate(2026, 1, 2), -7.0, u"Fictional Other"_s,
                      { new Allocation(&otherCategory, -7.0) }, {}));

    QCOMPARE(budgetData.countOperationsWithCategory(&category), 1);
    QCOMPARE(budgetData.countOperationsWithCategory(&otherCategory), 1);
    QCOMPARE(budgetData.countOperationsWithCategory(nullptr), 0);

    budgetData.set_currentTabIndex(2);
    budgetData.navigateToOperation(operation);
    QCOMPARE(budgetData.currentAccount(), account);
    QCOMPARE(account->currentOperation(), operation);
    QCOMPARE(budgetData.currentTabIndex(), 0);
  }

  void removesAndTakesAccounts() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* first = budgetData.createAccount(u"Fictional First"_s);
    auto* second = budgetData.createAccount(u"Fictional Second"_s);
    auto* third = budgetData.createAccount(u"Fictional Third"_s);
    QSignalSpy countSpy(&budgetData, &BudgetData::accountCountChanged);

    budgetData.set_currentAccount(second);
    budgetData.removeAccount(-1);
    budgetData.removeAccount(9);
    QCOMPARE(budgetData.rowCount(), 3);
    budgetData.removeAccount(0);
    QCOMPARE(budgetData.rowCount(), 2);
    QCOMPARE(budgetData.currentAccount(), second);
    QCOMPARE(budgetData.currentAccountIndex(), 0);
    QCOMPARE(countSpy.count(), 1);

    auto* taken = budgetData.takeAccount(second);
    QCOMPARE(taken, second);
    QVERIFY(taken->parent() == nullptr);
    QVERIFY(budgetData.currentAccount() == nullptr);
    delete taken;
    QVERIFY(budgetData.takeAccount(nullptr) == nullptr);
    QVERIFY(budgetData.takeAccount(first) == nullptr);
    budgetData.clearAccounts();
    QCOMPARE(budgetData.rowCount(), 0);
    budgetData.clearAccounts();
    QVERIFY(budgetData.currentAccount() == nullptr);
    Q_UNUSED(third);
  }
};

QTEST_GUILESS_MAIN(BudgetDataTest)
#include "BudgetDataTest.moc"
