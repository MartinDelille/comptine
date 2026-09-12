#include <QSignalSpy>
#include <QTest>

#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"

class AccountTest : public QObject {
  Q_OBJECT

private slots:
  void sortsOperationsAndCalculatesBalances() {
    Account account("Fictional Checking", nullptr);
    auto* oldest = account.addOperation(
        new Operation(&account, QDate(2026, 1, 1), 100.0, "Oldest"));
    auto* newer = account.addOperation(
        new Operation(&account, QDate(2026, 1, 3), -30.0, "Newer"));
    auto* sameDay = account.addOperation(
        new Operation(&account, QDate(2026, 1, 3), 20.0, "Same day"));

    QCOMPARE(account.operationAt(0), newer);
    QCOMPARE(account.operationAt(1), sameDay);
    QCOMPARE(account.operationAt(2), oldest);
    QCOMPARE(account.balanceAt(0), 90.0);
    QCOMPARE(account.balanceAt(1), 120.0);
    QCOMPARE(account.balanceAt(2), 100.0);
    QCOMPARE(account.currentBalance(), 90.0);
    QCOMPARE(account.balanceAt(-1), 0.0);
    QCOMPARE(account.balanceAt(5), 0.0);
    QVERIFY(account.hasOperation(QDate(2026, 1, 3), -30.0, "Newer"));
    QVERIFY(!account.hasOperation(QDate(2026, 1, 3), -31.0, "Newer"));
  }

  void replacesOperationsInOneModelUpdate() {
    Account account("Fictional Checking", nullptr);
    account.addOperation(new Operation(&account, QDate(2026, 1, 1), 100.0, "Old"));

    QSignalSpy resetSpy(&account, &QAbstractItemModel::modelReset);
    QSignalSpy operationSpy(&account, &Account::operationDataChanged);

    auto* oldest = new Operation(&account, QDate(2026, 2, 1), 100.0, "Oldest");
    auto* newest = new Operation(&account, QDate(2026, 2, 3), -30.0, "Newest");
    account.replaceOperations({ oldest, newest });

    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(operationSpy.count(), 1);
    QCOMPARE(account.rowCount(), 2);
    QCOMPARE(account.operationAt(0), newest);
    QCOMPARE(account.operationAt(1), oldest);
    QCOMPARE(account.balanceAt(0), 70.0);
    QCOMPARE(account.balanceAt(1), 100.0);
  }

  void selectionAndModelRolesExposeOperationState() {
    Account account("Fictional Checking", nullptr);
    auto* first = account.addOperation(
        new Operation(&account, QDate(2026, 2, 1), -10.0, "First"));
    auto* second = account.addOperation(
        new Operation(&account, QDate(2026, 2, 2), -20.0, "Second"));

    QVERIFY(!account.data(QModelIndex(), Account::LabelRole).isValid());
    const QModelIndex index = account.index(1, 0);
    QCOMPARE(account.data(index, Account::DateRole).toDate(), first->date());
    QCOMPARE(account.data(index, Account::AmountRole).toDouble(), -10.0);
    QCOMPARE(account.data(index, Account::LabelRole).toString(), QString("First"));
    QCOMPARE(account.data(index, Account::BalanceRole).toDouble(), -10.0);
    QCOMPARE(account.data(index, Account::OperationRole).value<Operation*>(), first);
    QVERIFY(!account.data(index, Qt::DisplayRole).isValid());

    account.selectAt(1);
    QCOMPARE(account.currentOperation(), first);
    QCOMPARE(account.selectionCount(), 1);
    QCOMPARE(account.selectedTotal(), -10.0);
    QVERIFY(account.isSelectedAt(1));
    account.toggleSelection(second);
    QCOMPARE(account.selectionCount(), 2);
    account.clearSelection();
    QCOMPARE(account.selectionCount(), 0);

    account.selectRange(-10, 10);
    QCOMPARE(account.selectionCount(), 2);
    QCOMPARE(account.selectedTotal(), -30.0);
    QVERIFY(account.selectedOperationsAsCsv().contains("First"));
    account.clearSelection();
    QVERIFY(account.selectedOperationsAsCsv().isEmpty());
  }

  void navigationAndImportSourcesHandleBoundaries() {
    Account account("Fictional Checking", nullptr);
    auto* first = account.addOperation(
        new Operation(&account, QDate(2026, 3, 1), -10.0, "First"));
    auto* second = account.addOperation(
        new Operation(&account, QDate(2026, 3, 2), -20.0, "Second"));
    account.select(second);
    account.previousOperation();
    QCOMPARE(account.currentOperation(), second);
    account.select(second);
    account.nextOperation(true);
    QCOMPARE(account.currentOperation(), first);
    QCOMPARE(account.selectionCount(), 2);
    account.previousOperation();
    QCOMPARE(account.currentOperation(), second);
    account.nextOperation();
    QCOMPARE(account.currentOperation(), first);
    QVERIFY(account.operationAt(-1) == nullptr);
    QVERIFY(account.operationAt(10) == nullptr);
    QVERIFY(account.operationIndex(nullptr) == -1);

    QSignalSpy sourceSpy(&account, &Account::importSourcePrefixesChanged);
    account.addImportSourcePrefix("");
    account.addImportSourcePrefix("fictional-bank");
    account.addImportSourcePrefix("fictional-bank");
    QCOMPARE(account.importSourcePrefixes(), QStringList({ "fictional-bank" }));
    account.setImportSourcePrefixes({ "fictional-one", "fictional-two" });
    QCOMPARE(sourceSpy.count(), 2);
    QCOMPARE(account.importSourcePrefixes().size(), 2);
  }

  void removeOperationUpdatesSelectionAndCurrentOperation() {
    Account account("Fictional Checking", nullptr);
    auto* operation = account.addOperation(
        new Operation(&account, QDate(2026, 4, 1), -10.0, "Removable"));
    account.select(operation);
    QVERIFY(account.removeOperation(operation));
    QCOMPARE(account.rowCount(), 0);
    QVERIFY(account.currentOperation() == nullptr);
    QCOMPARE(account.selectionCount(), 0);
    QVERIFY(!account.removeOperation(operation));
    QVERIFY(!account.removeOperation(nullptr));
    account.clear();
  }
};

QTEST_GUILESS_MAIN(AccountTest)
#include "AccountTest.moc"
