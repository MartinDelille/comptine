#include <QSignalSpy>
#include <QTest>
#include <QUndoStack>
#include <QUrl>

#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "services/BudgetData.h"

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

    auto* first = budgetData.createAccount("Fictional Checking");
    auto* second = budgetData.createAccount("Fictional Savings");
    QCOMPARE(budgetData.rowCount(), 2);
    QCOMPARE(budgetData.currentAccountIndex(), -1);
    budgetData.set_currentAccountIndex(1);
    QCOMPARE(budgetData.currentAccount(), second);
    QCOMPARE(budgetData.currentAccountIndex(), 1);

    QVERIFY(budgetData.at(-1) == nullptr);
    QVERIFY(budgetData.at(2) == nullptr);
    QCOMPARE(budgetData.accountByName("Fictional Checking"), first);
    QVERIFY(budgetData.accountByName("Missing") == nullptr);
    QCOMPARE(budgetData.accountIndex(first), 0);
    QCOMPARE(budgetData.accountIndex(nullptr), -1);

    const auto roles = budgetData.roleNames();
    QCOMPARE(roles.value(BudgetData::NameRole), QByteArray("name"));
    QCOMPARE(roles.value(BudgetData::OperationCountRole), QByteArray("operationCount"));
    QCOMPARE(roles.value(BudgetData::AccountRole), QByteArray("account"));
    QVERIFY(!budgetData.data(QModelIndex(), BudgetData::NameRole).isValid());
    QVERIFY(!budgetData.data(budgetData.index(5, 0), BudgetData::NameRole).isValid());
    QVERIFY(!budgetData.data(budgetData.index(0, 0), Qt::DisplayRole).isValid());
    QCOMPARE(budgetData.data(budgetData.index(0, 0), BudgetData::NameRole).toString(),
             QString("Fictional Checking"));
    QCOMPARE(budgetData.data(budgetData.index(0, 0), BudgetData::OperationCountRole).toInt(), 0);
    QCOMPARE(budgetData.data(budgetData.index(0, 0), BudgetData::AccountRole).value<Account*>(),
             first);
    QCOMPARE(budgetData.rowCount(budgetData.index(0, 0)), 0);
  }

  void matchesAccountsToImportUrls() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* account = budgetData.createAccount("Fictional Bank");
    account->addImportSourcePrefix("fictional-bank");
    budgetData.createAccount("Fictional Card");

    QCOMPARE(budgetData.suggestedAccountForUrl(QUrl::fromLocalFile("/tmp/Fictional Bank.csv")),
             QString("Fictional Bank"));
    QCOMPARE(budgetData.suggestedAccountForUrl(QUrl::fromLocalFile("/tmp/fictional-bank-2026.csv")),
             QString("Fictional Bank"));
    QCOMPARE(budgetData.suggestedAccountForUrl(QUrl::fromLocalFile("/tmp/unmatched.csv")),
             QString("unmatched"));
  }

  void navigatesOperationsAndCountsCategories() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    Category category("Fictional Category");
    Category otherCategory("Fictional Other");
    auto* account = budgetData.createAccount("Fictional Checking");
    auto* otherAccount = budgetData.createAccount("Fictional Savings");
    auto* operation = account->addOperation(
        new Operation(account, QDate(2026, 1, 1), -12.0, "Fictional Purchase",
                      {}, { new Allocation(&category, -12.0) }));
    otherAccount->addOperation(
        new Operation(otherAccount, QDate(2026, 1, 2), -7.0, "Fictional Other",
                      {}, { new Allocation(&otherCategory, -7.0) }));

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
    auto* first = budgetData.createAccount("Fictional First");
    auto* second = budgetData.createAccount("Fictional Second");
    auto* third = budgetData.createAccount("Fictional Third");
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
