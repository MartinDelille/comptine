#include <QSignalSpy>
#include <QTest>
#include <QUndoStack>
#include <QVariant>

#include "editor/OperationEditor.h"
#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "services/BudgetData.h"

using namespace Qt::StringLiterals;

class OperationEditorTest : public QObject {
  Q_OBJECT

private slots:
  void editingGuardsAndCancelRestoreState() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* account = budgetData.createAccount(u"Fictional Checking"_s);
    auto* operation = account->addOperation(
        new Operation(account, QDate(2026, 1, 15), -42.50, u"Fictional Purchase"_s, {}, u"Original details"_s));
    OperationEditor editor(budgetData, undoStack);

    QSignalSpy rejectedSpy(&editor, &OperationEditor::transactionRejected);
    QSignalSpy undoRejectedSpy(&editor, &OperationEditor::undoRejected);
    QSignalSpy redoRejectedSpy(&editor, &OperationEditor::redoRejected);

    QVERIFY(editor.beginEditing(operation));
    QVERIFY(editor.isEditing());
    QVERIFY(!editor.canUndo());
    QVERIFY(!editor.canRedo());
    QVERIFY(!editor.beginEditing(operation));
    editor.undo();
    editor.redo();
    QCOMPARE(rejectedSpy.count(), 1);
    QCOMPARE(undoRejectedSpy.count(), 1);
    QCOMPARE(redoRejectedSpy.count(), 1);

    editor.setLabel(operation, u"Changed label"_s);
    editor.setDetails(operation, u"Changed details"_s);
    editor.endEditing(false);

    QCOMPARE(operation->label(), u"Fictional Purchase"_s);
    QCOMPARE(operation->details(), u"Original details"_s);
    QVERIFY(!editor.isEditing());
    QCOMPARE(undoStack.count(), 1);

    editor.endEditing(false);
    QCOMPARE(rejectedSpy.count(), 2);
  }

  void scalarEditsAreUndoableAsOneTransaction() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* account = budgetData.createAccount(u"Fictional Checking"_s);
    auto* operation = account->addOperation(
        new Operation(account, QDate(2026, 1, 15), -42.50, u"Original label"_s, {}, u"Original details"_s));
    OperationEditor editor(budgetData, undoStack);

    QVERIFY(editor.beginEditing(operation));
    editor.setAmount(operation, -50.00);
    editor.setBudgetDate(operation, QDate(2026, 2, 1));
    editor.setDate(operation, QDate(2026, 1, 20));
    editor.setLabel(operation, u"Updated label"_s);
    editor.setDetails(operation, u"Updated details"_s);
    editor.endEditing(true);

    QCOMPARE(operation->amount(), -50.00);
    QCOMPARE(operation->date(), QDate(2026, 1, 20));
    QCOMPARE(operation->budgetDate(), QDate(2026, 2, 1));
    QCOMPARE(operation->label(), u"Updated label"_s);
    QCOMPARE(operation->details(), u"Updated details"_s);
    QCOMPARE(undoStack.count(), 1);
    QVERIFY(editor.canUndo());

    editor.undo();
    QCOMPARE(operation->amount(), -42.50);
    QCOMPARE(operation->date(), QDate(2026, 1, 15));
    QCOMPARE(operation->budgetDate(), QDate(2026, 1, 15));
    QCOMPARE(operation->label(), u"Original label"_s);
    QCOMPARE(operation->details(), u"Original details"_s);
    QVERIFY(editor.canRedo());

    editor.redo();
    QCOMPARE(operation->amount(), -50.00);
    QCOMPARE(operation->budgetDate(), QDate(2026, 2, 1));
  }

  void beginNewRequiresCurrentAccountAndCanBeCancelled() {
    QUndoStack emptyUndoStack;
    BudgetData emptyBudget(emptyUndoStack);
    OperationEditor emptyEditor(emptyBudget, emptyUndoStack);
    QVERIFY(emptyEditor.beginNew(QDate(2026, 1, 1), "No account"_L1, 10.0, ""_L1) == nullptr);

    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* account = budgetData.createAccount(u"Fictional Checking"_s);
    budgetData.set_currentAccount(account);
    OperationEditor editor(budgetData, undoStack);

    auto* operation = editor.beginNew(QDate(2026, 2, 1), u"New operation"_s, -20.0, u"Details"_s);
    QVERIFY(operation != nullptr);
    QCOMPARE(account->operations().size(), 1);
    QVERIFY(editor.isEditing());

    editor.endEditing(false);
    QCOMPARE(account->operations().size(), 0);
    QCOMPARE(undoStack.count(), 1);

    operation = editor.beginNew(QDate(2026, 2, 1), u"New operation"_s, -20.0, u"Details"_s);
    QVERIFY(operation != nullptr);
    editor.endEditing(true);
    QCOMPARE(account->operations().size(), 1);
    QCOMPARE(undoStack.count(), 1);
  }

  void addFiltersAllocationVariantsAndIsUndoable() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* account = budgetData.createAccount(u"Fictional Checking"_s);
    budgetData.set_currentAccount(account);
    Category category(u"Fictional Category"_s);
    auto* allocation = new Allocation(&category, -25.0);
    QVariantList values = { QVariant::fromValue(static_cast<QObject*>(allocation)), 42 };
    OperationEditor editor(budgetData, undoStack);

    editor.add(QDate(2026, 4, 1), u"Added operation"_s, -25.0, u"Details"_s, values);

    QCOMPARE(account->operations().size(), 1);
    auto* operation = account->operationAt(0);
    QCOMPARE(operation->allocations().size(), 1);
    QCOMPARE(operation->allocations().at(0)->category(), &category);
    QCOMPARE(operation->allocations().at(0)->amount(), -25.0);

    editor.undo();
    QCOMPARE(account->operations().size(), 0);
    editor.redo();
    QCOMPARE(account->operations().size(), 1);
  }

  void allocationEditingAndNoOpGuards() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* account = budgetData.createAccount(u"Fictional Checking"_s);
    Category food(u"Fictional Food"_s);
    Category travel(u"Fictional Travel"_s);
    auto* operation = account->addOperation(
        new Operation(account, QDate(2026, 1, 1), -100.0, u"Mixed purchase"_s));
    OperationEditor editor(budgetData, undoStack);

    editor.addAllocation(operation, &food, -60.0);
    editor.addAllocation(operation, &travel, -40.0);
    QCOMPARE(operation->allocations().size(), 2);

    const int commandCount = undoStack.count();
    editor.addAllocation(operation, &travel, 0.0);
    editor.removeAllocation(operation, -1);
    editor.removeAllocation(operation, 99);
    editor.setAllocationAmount(operation, 99, 1.0);
    editor.setAllocationCategory(operation, -1, &food);
    QCOMPARE(undoStack.count(), commandCount + 1);

    editor.setAllocationCategory(operation, 0, &travel);
    editor.setAllocationAmount(operation, 1, -30.0);
    editor.removeAllocation(operation, 0);
    editor.removeAllocation(operation, 1);
    QCOMPARE(operation->allocations().size(), 1);
    QCOMPARE(operation->allocations().at(0)->category(), &travel);
    QCOMPARE(operation->allocations().at(0)->amount(), -30.0);

    editor.addAllocation(nullptr, &food, 1.0);
    editor.removeAllocation(nullptr, 0);
    editor.setAllocationCategory(nullptr, 0, &food);
    editor.setAllocationAmount(nullptr, 0, 1.0);
    editor.normalizeAllocations(nullptr);
  }

  void addCounterpartAndDeleteSelectedAreUndoable() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* sourceAccount = budgetData.createAccount(u"Fictional Checking"_s);
    auto* targetAccount = budgetData.createAccount(u"Fictional Savings"_s);
    budgetData.set_currentAccount(sourceAccount);
    Category food(u"Fictional Food"_s);
    Category travel(u"Fictional Travel"_s);
    auto* operation = sourceAccount->addOperation(
        new Operation(sourceAccount, QDate(2026, 3, 1), -100.0, u"Transfer"_s,
                      { new Allocation(&food, -60.0), new Allocation(&travel, -40.0) }, u"Details"_s));
    OperationEditor editor(budgetData, undoStack);

    auto* counterpart = editor.createCounterpart(operation, targetAccount, u"Fictional Food"_s);
    QVERIFY(counterpart != nullptr);
    QCOMPARE(targetAccount->operations().size(), 1);
    QCOMPARE(counterpart->amount(), 60.0);
    QCOMPARE(counterpart->allocations().size(), 1);
    QCOMPARE(counterpart->allocations().at(0)->category(), &food);

    QVERIFY(editor.createCounterpart(nullptr, targetAccount, QString()) == nullptr);
    QVERIFY(editor.createCounterpart(operation, nullptr, QString()) == nullptr);

    budgetData.set_currentAccount(sourceAccount);
    sourceAccount->select(operation);
    editor.deleteSelected();
    QCOMPARE(sourceAccount->operations().size(), 0);
    undoStack.undo();
    QCOMPARE(sourceAccount->operations().size(), 1);
    QCOMPARE(sourceAccount->operationAt(0)->label(), u"Transfer"_s);
  }
};

QTEST_GUILESS_MAIN(OperationEditorTest)
#include "OperationEditorTest.moc"
