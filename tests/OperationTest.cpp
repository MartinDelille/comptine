#include <QSignalSpy>
#include <QTest>

#include "model/Category.h"
#include "model/Operation.h"

class OperationTest : public QObject {
  Q_OBJECT

private slots:
  void exposesAllocationModelRolesAndCategorySummaries() {
    Category food("Fictional Food");
    Category travel("Fictional Travel");
    Operation operation(nullptr, QDate(2026, 5, 1), -100.0, "Mixed purchase", "Details",
                        { new Allocation(&food, -60.0), new Allocation(&food, -10.0),
                          new Allocation(&travel, -30.0) });

    QCOMPARE(operation.rowCount(), 3);
    const QModelIndex index = operation.index(0, 0);
    QCOMPARE(operation.data(index, Operation::CategoryRole).value<Category*>(), &food);
    QCOMPARE(operation.data(index, Operation::AmountRole).toDouble(), -60.0);
    QVERIFY(!operation.data(QModelIndex(), Operation::AmountRole).isValid());
    QVERIFY(!operation.data(operation.index(20, 0), Operation::AmountRole).isValid());
    QVERIFY(!operation.data(index, Qt::DisplayRole).isValid());

    QCOMPARE(operation.allocatedAmount(), -100.0);
    QVERIFY(operation.isCategorized());
    QCOMPARE(operation.allocatedCategoryNames(), QStringList({ "Fictional Food", "Fictional Food", "Fictional Travel" }));
    QVERIFY(operation.categoryDisplay().contains("Fictional Food"));
    QVERIFY(operation.categoryDisplay().contains("Fictional Travel"));
    QCOMPARE(operation.amountForCategory(&food), -70.0);
    QCOMPARE(operation.amountForCategory(&travel), -30.0);
    QCOMPARE(operation.amountForCategory(nullptr), 0.0);
  }

  void budgetDateFallsBackAndSupportsExplicitValue() {
    const QDate operationDate(2026, 5, 15);
    Operation operation(nullptr, operationDate, 10.0, "Income");
    QCOMPARE(operation.budgetDate(), operationDate);

    QSignalSpy budgetDateSpy(&operation, &Operation::budgetDateChanged);
    const QDate budgetDate(2026, 6, 1);
    operation.set_budgetDate(budgetDate);
    QCOMPARE(operation.budgetDate(), budgetDate);
    QCOMPARE(budgetDateSpy.count(), 1);
    operation.set_budgetDate(budgetDate);
    QCOMPARE(budgetDateSpy.count(), 1);
    operation.set_budgetDate({});
    QCOMPARE(operation.budgetDate(), operationDate);
    QCOMPARE(budgetDateSpy.count(), 2);
  }

  void allocationReplacementAndClearingNotifyOnlyOnChanges() {
    Category category("Fictional Category");
    Operation operation(nullptr, {}, -50.0, "Purchase",
                        {}, { new Allocation(&category, -50.0) });
    QSignalSpy allocationsSpy(&operation, &Operation::allocationsChanged);

    operation.setAllocations({ new Allocation(&category, -50.0) });
    QCOMPARE(operation.rowCount(), 1);
    QCOMPARE(allocationsSpy.count(), 0);

    operation.setAllocations({ new Allocation(&category, -30.0) });
    QCOMPARE(operation.allocations().at(0)->amount(), -30.0);
    QCOMPARE(allocationsSpy.count(), 1);
    QVERIFY(!operation.isCategorized());

    operation.clearAllocations();
    QCOMPARE(operation.rowCount(), 0);
    QCOMPARE(allocationsSpy.count(), 2);
    operation.clearAllocations();
    QCOMPARE(allocationsSpy.count(), 2);
  }
};

QTEST_GUILESS_MAIN(OperationTest)
#include "OperationTest.moc"
