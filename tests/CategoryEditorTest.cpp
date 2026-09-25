#include <QDate>
#include <QTest>
#include <QUndoStack>

#include "editor/CategoryEditor.h"
#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "services/BudgetData.h"
#include "services/CategoryController.h"

using namespace Qt::StringLiterals;

class CategoryEditorTest : public QObject {
  Q_OBJECT

private slots:
  void createsAndEditsCategoriesUndoably() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    CategoryController controller(budgetData, undoStack);
    CategoryEditor editor(controller, budgetData, undoStack);

    auto* category = editor.edit(u"Fictional Food"_s, -250.0, nullptr, QDate(2026, 1, 1));
    QVERIFY(category != nullptr);
    QCOMPARE(controller.rowCount(), 1);
    QCOMPARE(category->name(), u"Fictional Food"_s);
    QCOMPARE(category->budgetLimitForMonth(QDate(2026, 1, 1)), -250.0);

    undoStack.undo();
    QCOMPARE(controller.rowCount(), 0);
    undoStack.redo();
    QCOMPARE(controller.rowCount(), 1);

    const int commandCount = undoStack.count();
    editor.edit(u"Fictional Food"_s, -250.0, category, QDate(2026, 1, 1));
    QCOMPARE(undoStack.count(), commandCount);

    editor.edit(u"Fictional Groceries"_s, -300.0, category, QDate(2026, 1, 1));
    QCOMPARE(category->name(), u"Fictional Groceries"_s);
    QCOMPARE(category->budgetLimitForMonth(QDate(2026, 1, 1)), -300.0);
    QCOMPARE(undoStack.count(), commandCount + 1);
    undoStack.undo();
    QCOMPARE(category->name(), u"Fictional Food"_s);
    QCOMPARE(category->budgetLimitForMonth(QDate(2026, 1, 1)), -250.0);
  }

  void monthlyBudgetLimitsInheritAndCanBeClearedUndoably() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    budgetData.set_budgetDate(QDate(2025, 1, 1));
    CategoryController controller(budgetData, undoStack);
    CategoryEditor editor(controller, budgetData, undoStack);

    auto* category = editor.edit(u"Fictional Budget"_s, 200.0, nullptr, QDate(2025, 1, 1));
    editor.edit(u"Fictional Budget"_s, 250.0, category, QDate(2025, 6, 1));
    editor.edit(u"Fictional Budget"_s, 220.0, category, QDate(2025, 3, 1));

    QCOMPARE(category->budgetLimitForMonth(QDate(2024, 12, 1)), 0.0);
    QCOMPARE(category->budgetLimitForMonth(QDate(2025, 2, 1)), 200.0);
    QCOMPARE(category->budgetLimitForMonth(QDate(2025, 5, 1)), 220.0);
    QCOMPARE(category->budgetLimitForMonth(QDate(2025, 6, 1)), 250.0);
    QVERIFY(category->hasBudgetLimitOverrideForMonth(QDate(2025, 6, 1)));

    editor.edit(u"Fictional Budget"_s, 0.0, category, QDate(2025, 6, 1), true);
    QCOMPARE(category->budgetLimitForMonth(QDate(2025, 6, 1)), 220.0);
    QVERIFY(!category->hasBudgetLimitOverrideForMonth(QDate(2025, 6, 1)));

    undoStack.undo();
    QCOMPARE(category->budgetLimitForMonth(QDate(2025, 6, 1)), 250.0);
    QVERIFY(category->hasBudgetLimitOverrideForMonth(QDate(2025, 6, 1)));
  }

  void createsAllocationsForKnownAndUnknownCategories() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    CategoryController controller(budgetData, undoStack);
    CategoryEditor editor(controller, budgetData, undoStack);
    auto* category = controller.addCategory(new Category(u"Fictional Bills"_s));

    auto* known = editor.createAllocation(u"Fictional Bills"_s, -75.0);
    QVERIFY(known != nullptr);
    QCOMPARE(known->category(), category);
    QCOMPARE(known->amount(), -75.0);
    delete known;

    auto* unknown = editor.createAllocation(u"Missing Category"_s, 12.0);
    QVERIFY(unknown != nullptr);
    QVERIFY(unknown->category() == nullptr);
    QCOMPARE(unknown->amount(), 12.0);
    delete unknown;
  }

  void monthlyAmountsAreUndoableAndIgnoreNoOps() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    CategoryController controller(budgetData, undoStack);
    CategoryEditor editor(controller, budgetData, undoStack);
    auto* category = controller.addCategory(new Category(u"Fictional Savings"_s));
    const QDate date(2026, 2, 1);

    editor.setSaveAmount(nullptr, date, 10.0);
    editor.setReportAmount(nullptr, date, 10.0);
    QCOMPARE(undoStack.count(), 0);

    editor.setSaveAmount(category, date, 10.0);
    QCOMPARE(category->monthRecord(QDate(2026, 2, 1)).saveAmount, 10.0);
    QCOMPARE(undoStack.count(), 1);
    editor.setSaveAmount(category, date, 10.0);
    QCOMPARE(undoStack.count(), 1);

    editor.setReportAmount(category, date, 25.0);
    QCOMPARE(category->monthRecord(QDate(2026, 2, 1)).reportAmount, 25.0);
    QCOMPARE(undoStack.count(), 1);
    editor.setReportAmount(category, date, 25.0);
    QCOMPARE(undoStack.count(), 1);

    undoStack.undo();
    QCOMPARE(category->monthRecord(QDate(2026, 2, 1)).reportAmount, 0.0);
    QCOMPARE(category->monthRecord(QDate(2026, 2, 1)).saveAmount, 0.0);
  }

  void removingCategoryRemovesAllocationsAndUndoRestoresBoth() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    CategoryController controller(budgetData, undoStack);
    CategoryEditor editor(controller, budgetData, undoStack);
    auto* category = controller.addCategory(new Category(u"Fictional Travel"_s));
    auto* otherCategory = controller.addCategory(new Category(u"Fictional Food"_s));
    controller.set_current(category);
    auto* account = budgetData.createAccount(u"Fictional Checking"_s);
    auto* operation = account->addOperation(
        new Operation(account, QDate(2026, 3, 1), -100.0, u"Mixed purchase"_s,
                      { new Allocation(category, -60.0), new Allocation(otherCategory, -40.0) }, {}));

    editor.remove(nullptr);
    QCOMPARE(controller.rowCount(), 2);
    editor.remove(category);
    QCOMPARE(controller.rowCount(), 1);
    QVERIFY(controller.getCategoryByName("Fictional Travel"_L1) == nullptr);
    QCOMPARE(operation->allocations().size(), 1);
    QCOMPARE(operation->allocations().at(0)->category(), otherCategory);
    QVERIFY(controller.current() != category);

    undoStack.undo();
    QCOMPARE(operation->allocations().size(), 2);
    undoStack.undo();
    QCOMPARE(controller.rowCount(), 2);
    QVERIFY(controller.getCategoryByName("Fictional Travel"_L1) != nullptr);
  }
};

QTEST_GUILESS_MAIN(CategoryEditorTest)
#include "CategoryEditorTest.moc"
