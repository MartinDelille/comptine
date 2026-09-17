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

  void allocationChangesEmitOperationDataChanged() {
    Account account("Fictional Checking", nullptr);
    Category category("Fictional Groceries");
    auto* operation = account.addOperation(
        new Operation(&account, QDate(2026, 2, 1), -30.0, "Purchase"));
    QSignalSpy operationSpy(&account, &Account::operationDataChanged);

    operation->setAllocations({ new Allocation(&category, -30.0) });

    QCOMPARE(operationSpy.count(), 1);
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

  void handlesInvalidInputsAndDataUpdates() {
    Account account("Fictional Checking", nullptr);
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
        new Operation(&account, QDate(2026, 5, 1), 25.0, "Valid"), false);
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
    Account account("Fictional Checking", nullptr);
    auto* oldest = account.addOperation(
        new Operation(&account, QDate(2026, 1, 1), 10.0, "Oldest"), false);
    auto* newest = account.addOperation(
        new Operation(&account, QDate(2026, 1, 3), 30.0, "Newest"), false);
    auto* middle = account.addOperation(
        new Operation(&account, QDate(2026, 1, 2), 20.0, "Middle"), false);

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
    newest->set_label("Newest updated");
    newest->set_details("Fictional details");
    newest->set_budgetDate(QDate(2026, 1, 5));
    QCOMPARE(dataSpy.count(), 5);
    QCOMPARE(balanceSpy.count(), 1);
    QCOMPARE(account.currentBalance(), 65.0);
  }

  void coversSelectionModesAndCsvFormatting() {
    Account account("Fictional Checking", nullptr);
    auto* first = account.addOperation(
        new Operation(&account, QDate(2026, 6, 1), -10.5, "First, \"quoted\""));
    auto* second = account.addOperation(
        new Operation(&account, QDate(2026, 6, 2), 20.0, "Second"));
    auto* third = account.addOperation(
        new Operation(&account, QDate(2026, 6, 3), -5.0, "Third"));
    Category category("Fictional Food");
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
    QVERIFY(csv.contains("First, \"\"quoted\"\""));
    QVERIFY(csv.contains("-10.50"));
    QVERIFY(csv.contains("Fictional Food"));
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
    Account account("Fictional Checking", nullptr);
    auto* operation = account.addOperation(
        new Operation(&account, QDate(2026, 7, 1), 40.0, "Selected"));
    account.select(operation);
    account.refresh();

    QSignalSpy resetSpy(&account, &QAbstractItemModel::modelReset);
    QSignalSpy selectionSpy(&account, &Account::selectionChanged);
    QSignalSpy currentSpy(&account, &Account::currentOperationChanged);
    QSignalSpy balanceSpy(&account, &Account::balanceChanged);
    auto* replacement = new Operation(nullptr, QDate(2026, 7, 2), 5.0, "Replacement");
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
