#include <QSignalSpy>
#include <QTest>

#include "model/Category.h"
#include "model/Operation.h"

using namespace Qt::StringLiterals;

class OperationTest : public QObject {
  Q_OBJECT

private slots:
  void exposesAllocationModelRolesAndCategorySummaries() {
    Category food(u"Fictional Food"_s);
    Category travel(u"Fictional Travel"_s);
    Operation operation(nullptr, QDate(2026, 5, 1), -100.0, u"Mixed purchase"_s,
                        { new Allocation(&food, -60.0), new Allocation(&food, -10.0),
                          new Allocation(&travel, -30.0) },
                        u"Details"_s);

    QCOMPARE(operation.rowCount(), 3);
    const QModelIndex index = operation.index(0, 0);
    QCOMPARE(operation.data(index, Operation::CategoryRole).value<Category*>(), &food);
    QCOMPARE(operation.data(index, Operation::AmountRole).toDouble(), -60.0);
    QVERIFY(!operation.data(QModelIndex(), Operation::AmountRole).isValid());
    QVERIFY(!operation.data(operation.index(20, 0), Operation::AmountRole).isValid());
    QVERIFY(!operation.data(index, Qt::DisplayRole).isValid());

    QCOMPARE(operation.allocatedAmount(), -100.0);
    QVERIFY(operation.isCategorized());
    QCOMPARE(operation.allocatedCategoryNames(), QStringList({ "Fictional Food"_L1, "Fictional Food"_L1, "Fictional Travel"_L1 }));
    QVERIFY(operation.categoryDisplay().contains("Fictional Food"_L1));
    QVERIFY(operation.categoryDisplay().contains("Fictional Travel"_L1));
    QCOMPARE(operation.amountForCategory(&food), -70.0);
    QCOMPARE(operation.amountForCategory(&travel), -30.0);
    QCOMPARE(operation.amountForCategory(nullptr), 0.0);
  }

  void coversModelMetadataAndNullAllocations() {
    Category category(u"Fictional Category"_s);
    Operation operation(nullptr, {}, 25.0, u"Income"_s,
                        { nullptr, new Allocation(nullptr, 5.0),
                          new Allocation(&category, 20.0) },
                        {});

    QCOMPARE(operation.rowCount(operation.index(0)), 0);
    QCOMPARE(operation.roleNames().value(Operation::CategoryRole), "category"_ba);
    QCOMPARE(operation.roleNames().value(Operation::AmountRole), "amount"_ba);
    QVERIFY(!operation.data(operation.index(0), Operation::CategoryRole).isValid());
    QCOMPARE(operation.data(operation.index(1), Operation::CategoryRole).value<const Category*>(), nullptr);
    QCOMPARE(operation.allocatedAmount(), 25.0);
    QCOMPARE(operation.allocatedCategoryNames(), QStringList({ "Fictional Category"_L1 }));
    QCOMPARE(operation.categoryDisplay(), u"Fictional Category"_s);
    QCOMPARE(operation.amountForCategory(&category), 20.0);
    QCOMPARE(operation.amountForCategory(nullptr), 0.0);
  }

  void comparesAllocationsIncludingNullAndDifferentValues() {
    Category first(u"Fictional First"_s);
    Category second(u"Fictional Second"_s);
    Allocation matching(&first, 10.0);
    Allocation differentAmount(&first, 11.0);
    Allocation differentCategory(&second, 10.0);
    Operation operation(nullptr, {}, 10.0, u"Purchase"_s, { new Allocation(&first, 10.0) }, {});

    QVERIFY(operation.sameAllocations({ &matching }));
    QVERIFY(!operation.sameAllocations({}));
    QVERIFY(!operation.sameAllocations({ &differentAmount }));
    QVERIFY(!operation.sameAllocations({ &differentCategory }));

    Operation nullOperation(nullptr, {}, 0.0, u"Empty"_s, { nullptr }, {});
    QVERIFY(nullOperation.sameAllocations({ nullptr }));
    QVERIFY(!nullOperation.sameAllocations({ &matching }));
    QVERIFY(!nullOperation.sameAllocations({ nullptr, nullptr }));

    QVERIFY(matching != differentAmount);
  }

  void budgetDateFallsBackAndSupportsExplicitValue() {
    const QDate operationDate(2026, 5, 15);
    Operation operation(nullptr, operationDate, 10.0, u"Income"_s);
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
    Category category(u"Fictional Category"_s);
    Operation operation(nullptr, {}, -50.0, u"Purchase"_s,
                        { new Allocation(&category, -50.0) }, {});
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

  void categoryRenameUpdatesCategorySummary() {
    Category category(u"Fictional Old Name"_s);
    Operation operation(nullptr, {}, -50.0, u"Purchase"_s,
                        { new Allocation(&category, -50.0) }, {});
    QSignalSpy allocationsSpy(&operation, &Operation::allocationsChanged);

    category.set_name(u"Fictional New Name"_s);

    QCOMPARE(operation.categoryDisplay(), u"Fictional New Name"_s);
    QCOMPARE(operation.allocatedCategoryNames(), QStringList({ "Fictional New Name"_L1 }));
    QCOMPARE(allocationsSpy.count(), 1);
  }

  void categorySignalsFollowAllocationReplacement() {
    Category oldCategory(u"Fictional Old"_s);
    Category newCategory(u"Fictional New"_s);
    Operation operation(nullptr, {}, -10.0, u"Purchase"_s,
                        { new Allocation(&oldCategory, -10.0) }, {});
    QSignalSpy allocationsSpy(&operation, &Operation::allocationsChanged);

    operation.setAllocations({ new Allocation(&newCategory, -10.0) });
    QCOMPARE(allocationsSpy.count(), 1);

    oldCategory.set_name(u"Fictional Old Renamed"_s);
    QCOMPARE(allocationsSpy.count(), 1);
    newCategory.set_name(u"Fictional New Renamed"_s);
    QCOMPARE(allocationsSpy.count(), 2);
  }
};

QTEST_GUILESS_MAIN(OperationTest)
#include "OperationTest.moc"
