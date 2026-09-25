#include <QSignalSpy>
#include <QTest>

#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"

using namespace Qt::StringLiterals;

class AccountTest : public QObject {
  Q_OBJECT

private slots:
  void sortsOperationsAndCalculatesBalances() {
    Account account(u"Fictional Checking"_s, nullptr);
    auto* oldest = account.addOperation(
        new Operation(&account, QDate(2026, 1, 1), 100.0, u"Oldest"_s));
    auto* newer = account.addOperation(
        new Operation(&account, QDate(2026, 1, 3), -30.0, u"Newer"_s));
    auto* sameDay = account.addOperation(
        new Operation(&account, QDate(2026, 1, 3), 20.0, u"Same day"_s));

    QCOMPARE(account.operationAt(0), newer);
    QCOMPARE(account.operationAt(1), sameDay);
    QCOMPARE(account.operationAt(2), oldest);
    QCOMPARE(account.balanceAt(0), 90.0);
    QCOMPARE(account.balanceAt(1), 120.0);
    QCOMPARE(account.balanceAt(2), 100.0);
    QCOMPARE(account.currentBalance(), 90.0);
    QCOMPARE(account.balanceAt(-1), 0.0);
    QCOMPARE(account.balanceAt(5), 0.0);
    QVERIFY(account.hasOperation(QDate(2026, 1, 3), -30.0, "Newer"_L1));
    QVERIFY(!account.hasOperation(QDate(2026, 1, 3), -31.0, "Newer"_L1));
  }

  void replacesOperationsInOneModelUpdate() {
    Account account(u"Fictional Checking"_s, nullptr);
    account.addOperation(new Operation(&account, QDate(2026, 1, 1), 100.0, u"Old"_s));

    QSignalSpy resetSpy(&account, &QAbstractItemModel::modelReset);
    QSignalSpy operationSpy(&account, &Account::operationDataChanged);

    auto* oldest = new Operation(&account, QDate(2026, 2, 1), 100.0, u"Oldest"_s);
    auto* newest = new Operation(&account, QDate(2026, 2, 3), -30.0, u"Newest"_s);
    account.replaceOperations({ oldest, newest });

    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(operationSpy.count(), 1);
    QCOMPARE(account.rowCount(), 2);
    QCOMPARE(account.operationAt(0), newest);
    QCOMPARE(account.operationAt(1), oldest);
    QCOMPARE(account.balanceAt(0), 70.0);
    QCOMPARE(account.balanceAt(1), 100.0);
  }

  void allocationChangesEmitOperationDataChanged() {
    Account account(u"Fictional Checking"_s, nullptr);
    Category category(u"Fictional Groceries"_s);
    auto* operation = account.addOperation(
        new Operation(&account, QDate(2026, 2, 1), -30.0, u"Purchase"_s));
    QSignalSpy operationSpy(&account, &Account::operationDataChanged);

    operation->setAllocations({ new Allocation(&category, -30.0) });

    QCOMPARE(operationSpy.count(), 1);
  }

  void selectionAndModelRolesExposeOperationState() {
    Account account(u"Fictional Checking"_s, nullptr);
    auto* first = account.addOperation(
        new Operation(&account, QDate(2026, 2, 1), -10.0, u"First"_s));
    auto* second = account.addOperation(
        new Operation(&account, QDate(2026, 2, 2), -20.0, u"Second"_s));

    QVERIFY(!account.data(QModelIndex(), Account::LabelRole).isValid());
    const QModelIndex index = account.index(1, 0);
    QCOMPARE(account.data(index, Account::DateRole).toDate(), first->date());
    QCOMPARE(account.data(index, Account::AmountRole).toDouble(), -10.0);
    QCOMPARE(account.data(index, Account::LabelRole).toString(), u"First"_s);
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
    QVERIFY(account.selectedOperationsAsCsv().contains("First"_L1));
    account.clearSelection();
    QVERIFY(account.selectedOperationsAsCsv().isEmpty());
  }

  void navigationAndImportSourcesHandleBoundaries() {
    Account account(u"Fictional Checking"_s, nullptr);
    auto* first = account.addOperation(
        new Operation(&account, QDate(2026, 3, 1), -10.0, u"First"_s));
    auto* second = account.addOperation(
        new Operation(&account, QDate(2026, 3, 2), -20.0, u"Second"_s));
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
    account.addImportSourcePrefix(""_L1);
    account.addImportSourcePrefix(u"fictional-bank"_s);
    account.addImportSourcePrefix(u"fictional-bank"_s);
    QCOMPARE(account.importSourcePrefixes(), QStringList({ "fictional-bank"_L1 }));
    account.setImportSourcePrefixes({ "fictional-one"_L1, "fictional-two"_L1 });
    QCOMPARE(sourceSpy.count(), 2);
    QCOMPARE(account.importSourcePrefixes().size(), 2);
  }

  void removeOperationUpdatesSelectionAndCurrentOperation() {
    Account account(u"Fictional Checking"_s, nullptr);
    auto* operation = account.addOperation(
        new Operation(&account, QDate(2026, 4, 1), -10.0, u"Removable"_s));
    account.select(operation);
    QVERIFY(account.removeOperation(operation));
    QCOMPARE(account.rowCount(), 0);
    QVERIFY(account.currentOperation() == nullptr);
    QCOMPARE(account.selectionCount(), 0);
    QVERIFY(!account.removeOperation(operation));
    QVERIFY(!account.removeOperation(nullptr));
    account.clear();
  }

  void handlesInvalidInputsAndDataUpdates() {
    Account account(u"Fictional Checking"_s, nullptr);
    QVERIFY(account.addOperation(nullptr) == nullptr);
    QCOMPARE(account.rowCount(), 0);
    QCOMPARE(account.rowCount(account.index(0, 0)), 0);
    QCOMPARE(account.currentOperationIndex(), -1);
    QCOMPARE(account.currentBalance(), 0.0);
    QCOMPARE(account.selectedTotal(), 0.0);
    QVERIFY(!account.isSelected(nullptr));
    QVERIFY(!account.isSelectedAt(-1));
    QVERIFY(!account.setData(QModelIndex(), true, Account::SelectedRole));
    QVERIFY(!account.setData(QModelIndex(), true, Qt::DisplayRole));

    auto* operation = account.addOperation(
        new Operation(&account, QDate(2026, 5, 1), 25.0, u"Valid"_s), false);
    const QModelIndex staleIndex = account.index(0, 0);
    QVERIFY(!account.data(staleIndex, Account::DateRole).isNull());
    account.clear();
    QVERIFY(!account.data(staleIndex, Account::DateRole).isValid());
    QVERIFY(account.operationIndex(operation) == -1);
    account.set_currentOperationIndex(0);
    account.toggleSelectionAt(0);
    account.selectAt(0);
    account.selectOperations({ nullptr }, false);
    account.selectRange(0, 1);
    account.selectAll();
    account.clearSelection();
  }

  void supportsUnsortedInsertionAndResorting() {
    Account account(u"Fictional Checking"_s, nullptr);
    auto* oldest = account.addOperation(
        new Operation(&account, QDate(2026, 1, 1), 10.0, u"Oldest"_s), false);
    auto* newest = account.addOperation(
        new Operation(&account, QDate(2026, 1, 3), 30.0, u"Newest"_s), false);
    auto* middle = account.addOperation(
        new Operation(&account, QDate(2026, 1, 2), 20.0, u"Middle"_s), false);

    QCOMPARE(account.operationAt(0), oldest);
    account.select(middle);
    QSignalSpy resetSpy(&account, &QAbstractItemModel::modelReset);
    QSignalSpy selectionSpy(&account, &Account::selectionChanged);
    QSignalSpy currentSpy(&account, &Account::currentOperationChanged);
    account.sortOperations();

    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(account.operationAt(0), newest);
    QCOMPARE(account.operationAt(1), middle);
    QCOMPARE(account.operationAt(2), oldest);
    QCOMPARE(account.currentOperationIndex(), 1);
    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(currentSpy.count(), 1);
    QCOMPARE(account.balanceAt(0), 60.0);

    QSignalSpy dataSpy(&account, &Account::operationDataChanged);
    QSignalSpy balanceSpy(&account, &Account::balanceChanged);
    newest->set_amount(35.0);
    newest->set_date(QDate(2026, 1, 4));
    newest->set_label(u"Newest updated"_s);
    newest->set_details(u"Fictional details"_s);
    newest->set_budgetDate(QDate(2026, 1, 5));
    QCOMPARE(dataSpy.count(), 5);
    QCOMPARE(balanceSpy.count(), 1);
    QCOMPARE(account.currentBalance(), 65.0);
  }

  void coversSelectionModesAndCsvFormatting() {
    Account account(u"Fictional Checking"_s, nullptr);
    auto* first = account.addOperation(
        new Operation(&account, QDate(2026, 6, 1), -10.5, u"First, \"quoted\""_s));
    auto* second = account.addOperation(
        new Operation(&account, QDate(2026, 6, 2), 20.0, u"Second"_s));
    auto* third = account.addOperation(
        new Operation(&account, QDate(2026, 6, 3), -5.0, u"Third"_s));
    Category category(u"Fictional Food"_s);
    second->setAllocations({ new Allocation(&category, 12.0) });
    third->setAllocations({ new Allocation(&category, -5.0) });

    account.selectOperations({ first, first, nullptr }, false);
    QCOMPARE(account.currentOperation(), first);
    account.selectOperations({ second }, true);
    QCOMPARE(account.selectionCount(), 2);
    account.clearCurrentOperation();
    account.selectOperations({ third }, true);
    QCOMPARE(account.selectionCount(), 3);
    QVERIFY(account.selectedOperations().contains(third));

    const QString csv = account.selectedOperationsAsCsv();
    QVERIFY(csv.contains("First, \"\"quoted\"\""_L1));
    QVERIFY(csv.contains("-10.50"_L1));
    QVERIFY(csv.contains("Fictional Food"_L1));
    QCOMPARE(account.countOperationsWithCategory(&category), 2);
    QCOMPARE(account.countOperationsWithCategory(nullptr), 0);

    const QModelIndex firstIndex = account.index(account.operationIndex(first), 0);
    account.setData(firstIndex, false, Account::SelectedRole);
    QVERIFY(!account.isSelected(first));
    account.setData(firstIndex, true, Account::SelectedRole);
    QVERIFY(account.isSelected(first));
    account.toggleSelection(first);
    QVERIFY(!account.isSelected(first));
  }

  void refreshesAndReplacesWithStateNotifications() {
    Account account(u"Fictional Checking"_s, nullptr);
    auto* operation = account.addOperation(
        new Operation(&account, QDate(2026, 7, 1), 40.0, u"Selected"_s));
    account.select(operation);
    account.refresh();

    QSignalSpy resetSpy(&account, &QAbstractItemModel::modelReset);
    QSignalSpy selectionSpy(&account, &Account::selectionChanged);
    QSignalSpy currentSpy(&account, &Account::currentOperationChanged);
    QSignalSpy balanceSpy(&account, &Account::balanceChanged);
    auto* replacement = new Operation(nullptr, QDate(2026, 7, 2), 5.0, u"Replacement"_s);
    account.replaceOperations({ replacement });

    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(currentSpy.count(), 1);
    QCOMPARE(balanceSpy.count(), 1);
    QCOMPARE(account.currentOperation(), nullptr);
    QCOMPARE(account.selectionCount(), 0);
    QCOMPARE(account.currentBalance(), 5.0);

    account.clearCurrentOperation();
    account.clearSelection();
    account.clear();
    QCOMPARE(account.rowCount(), 0);
  }
};

QTEST_GUILESS_MAIN(AccountTest)
#include "AccountTest.moc"
