#include <QTest>
#include <QUndoStack>

#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "services/BudgetData.h"
#include "services/CategoryController.h"

class CategoryControllerTest : public QObject {
  Q_OBJECT

private slots:
  void categoriesAreSortedAndDuplicateNamesAreRejected() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    CategoryController controller(budgetData, undoStack);

    QVERIFY(controller.addCategory(nullptr) == nullptr);
    auto* zulu = controller.addCategory(new Category("Fictional Zulu"));
    auto* alpha = controller.addCategory(new Category("Fictional Alpha"));
    QVERIFY(zulu != nullptr);
    QVERIFY(alpha != nullptr);
    QVERIFY(controller.addCategory(new Category("Fictional Alpha")) == nullptr);
    QCOMPARE(controller.categoryNames(), QStringList({ "Fictional Alpha", "Fictional Zulu" }));
    QCOMPARE(controller.categoryIndex(alpha), 0);
    QCOMPARE(controller.categoryIndex(nullptr), -1);
    QCOMPARE(controller.getCategoryByName("Fictional Zulu"), zulu);
    QVERIFY(controller.getCategoryByName("fictional zulu") == nullptr);
    QVERIFY(controller.at(-1) == nullptr);
    QVERIFY(controller.at(5) == nullptr);

    controller.set_currentIndex(0);
    QCOMPARE(controller.current(), alpha);
    controller.set_currentIndex(20);
    QVERIFY(controller.current() == nullptr);

    auto* taken = controller.takeCategoryByName("fIcTiOnAl zUlU");
    QCOMPARE(taken, zulu);
    delete taken;
    QVERIFY(controller.takeCategoryByName("missing") == nullptr);
    QCOMPARE(controller.rowCount(QModelIndex()), 1);
    QCOMPARE(controller.rowCount(controller.index(0, 0)), 0);

    controller.clear();
    QCOMPARE(controller.rowCount(), 0);
    controller.clear();
  }

  void aggregatesAndModelRolesUseBudgetMonthData() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    CategoryController controller(budgetData, undoStack);
    const QDate budgetDate(2026, 6, 15);
    budgetData.set_budgetDate(budgetDate);
    auto* income = controller.addCategory(new Category("Fictional Income"));
    auto* expense = controller.addCategory(new Category("Fictional Expense"));
    expense->setMonthRecord(QDate(2026, 6, 1), { 20.0, 30.0, -300.0 });
    income->setMonthRecord(QDate(2026, 6, 1), { 0.0, 0.0, 500.0 });

    auto* account = budgetData.createAccount("Fictional Checking");
    account->addOperation(new Operation(account, QDate(2026, 6, 10), 500.0,
                                        "Fictional Salary",
                                        { new Allocation(income, 500.0) }, {}));
    account->addOperation(new Operation(account, QDate(2026, 6, 12), -100.0,
                                        "Fictional Purchase",
                                        { new Allocation(expense, -100.0) }, {}));
    account->addOperation(new Operation(account, QDate(2026, 5, 31), -50.0,
                                        "Previous month",
                                        { new Allocation(expense, -50.0) }, {}));

    QCOMPARE(controller.totalIncome(), 500.0);
    QCOMPARE(controller.totalExpense(), 300.0);
    QCOMPARE(controller.totalToSave(), 20.0);
    QCOMPARE(controller.totalToReport(), 30.0);
    QCOMPARE(controller.totalFromReport(), 0.0);
    QCOMPARE(controller.netReport(), 30.0);
    QCOMPARE(controller.spentInCategory(income, budgetDate), 500.0);
    QCOMPARE(controller.spentInCategory(expense, budgetDate), -100.0);
    QCOMPARE(controller.spentInCategory(nullptr, budgetDate), 0.0);
    QCOMPARE(controller.leftoverForCategory(income, budgetDate), 0.0);
    QCOMPARE(controller.leftoverForCategory(expense, budgetDate), 200.0);
    QCOMPARE(controller.leftoverForCategory(nullptr, budgetDate), 0.0);
    QCOMPARE(controller.accumulatedLeftover("Missing", budgetDate), 0.0);

    QCOMPARE(controller.accumulatedLeftover("Fictional Expense", budgetDate), 30.0);
    QCOMPARE(controller.data(controller.index(controller.categoryIndex(expense), 0),
                             CategoryController::AccumulatedRole)
                 .toDouble(),
             30.0);
    QCOMPARE(controller.data(controller.index(controller.categoryIndex(expense), 0),
                             CategoryController::LeftoverRole)
                 .toDouble(),
             200.0);

    const QModelIndex index = controller.index(controller.categoryIndex(expense), 0);
    QCOMPARE(controller.data(index, CategoryController::CategoryRole).value<Category*>(), expense);
    QCOMPARE(controller.data(index, CategoryController::AmountRole).toDouble(), -100.0);
    QCOMPARE(controller.data(index, CategoryController::SaveAmountRole).toDouble(), 20.0);
    QCOMPARE(controller.data(index, CategoryController::ReportAmountRole).toDouble(), 30.0);
    QCOMPARE(controller.data(index, CategoryController::BudgetLimitRole).toDouble(), -300.0);
    QVERIFY(!controller.data(QModelIndex(), CategoryController::AmountRole).isValid());

    const auto roles = controller.roleNames();
    QCOMPARE(roles.value(CategoryController::CategoryRole), QByteArray("category"));
    QCOMPARE(roles.value(CategoryController::AccumulatedRole), QByteArray("accumulated"));
    QVERIFY(!controller.isBalanced(controller.categoryIndex(expense)));
    QVERIFY(controller.isBalanced(-1));
    QCOMPARE(controller.balancedCount(), 1);

    expense->setMonthRecord(QDate(2026, 6, 1), { 170.0, 30.0, -300.0 });
    QVERIFY(controller.isBalanced(controller.categoryIndex(expense)));
    QCOMPARE(controller.balancedCount(), 2);
  }

  void operationsForCategoryReturnsOnlyMatchingMonthAndCategory() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    CategoryController controller(budgetData, undoStack);
    const QDate date(2026, 7, 1);
    auto* category = controller.addCategory(new Category("Fictional Category"));
    auto* other = controller.addCategory(new Category("Fictional Other"));
    auto* account = budgetData.createAccount("Fictional Checking");
    auto* older = account->addOperation(
        new Operation(account, QDate(2026, 7, 2), -20.0, "Older",
                      { new Allocation(category, -20.0) }, {}));
    auto* newer = account->addOperation(
        new Operation(account, QDate(2026, 7, 20), -30.0, "Newer",
                      { new Allocation(category, -15.0), new Allocation(other, -15.0) }, {}));
    auto* budgetDated = account->addOperation(
        new Operation(account, QDate(2026, 8, 1), -10.0, "Budget dated",
                      { new Allocation(category, -10.0) }, {}));
    budgetDated->set_budgetDate(date);
    account->addOperation(new Operation(account, QDate(2026, 7, 10), 0.0,
                                        "Zero allocation",
                                        { new Allocation(category, 0.0) }, {}));
    account->addOperation(new Operation(account, QDate(2026, 6, 30), -40.0,
                                        "Wrong month",
                                        { new Allocation(category, -40.0) }, {}));

    const QVariantList result = controller.operationsForCategory(category, date);
    QCOMPARE(result.size(), 3);
    QCOMPARE(result.at(0).toMap().value("operation").value<Operation*>(), budgetDated);
    QCOMPARE(result.at(0).toMap().value("amount").toDouble(), -10.0);
    QCOMPARE(result.at(0).toMap().value("totalAmount").toDouble(), -10.0);
    QCOMPARE(result.at(0).toMap().value("accountName").toString(), QString("Fictional Checking"));
    QCOMPARE(result.at(0).toMap().value("label").toString(), QString("Budget dated"));
    QCOMPARE(result.at(0).toMap().value("date").toDate(), QDate(2026, 8, 1));
    QCOMPARE(result.at(0).toMap().value("budgetDate").toDate(), date);
    QVERIFY(result.at(0).toMap().value("isCategorized").toBool());
    QCOMPARE(result.at(1).toMap().value("operation").value<Operation*>(), newer);
    QCOMPARE(result.at(1).toMap().value("amount").toDouble(), -15.0);
    QCOMPARE(result.at(2).toMap().value("operation").value<Operation*>(), older);
    QCOMPARE(controller.operationsForCategory(nullptr, date).size(), 0);
  }
};

QTEST_GUILESS_MAIN(CategoryControllerTest)
#include "CategoryControllerTest.moc"
