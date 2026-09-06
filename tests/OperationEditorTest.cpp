#include <QSignalSpy>
#include <QTest>
#include <QUndoStack>
#include <QVariant>

#include "editor/OperationEditor.h"
#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "services/BudgetData.h"

class OperationEditorTest : public QObject {
  Q_OBJECT

private slots:
  void editingGuardsAndCancelRestoreState() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* account = budgetData.createAccount("Fictional Checking");
    auto* operation = account->addOperation(
        new Operation(account, QDate(2026, 1, 15), -42.50, "Fictional Purchase", "Original details"));
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

    editor.setLabel(operation, "Changed label");
    editor.setDetails(operation, "Changed details");
    editor.endEditing(false);

    QCOMPARE(operation->label(), QString("Fictional Purchase"));
    QCOMPARE(operation->details(), QString("Original details"));
    QVERIFY(!editor.isEditing());
    QCOMPARE(undoStack.count(), 1);

    editor.endEditing(false);
    QCOMPARE(rejectedSpy.count(), 2);
  }

  void scalarEditsAreUndoableAsOneTransaction() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* account = budgetData.createAccount("Fictional Checking");
    auto* operation = account->addOperation(
        new Operation(account, QDate(2026, 1, 15), -42.50, "Original label", "Original details"));
    OperationEditor editor(budgetData, undoStack);

    QVERIFY(editor.beginEditing(operation));
    editor.setAmount(operation, -50.00);
    editor.setBudgetDate(operation, QDate(2026, 2, 1));
    editor.setDate(operation, QDate(2026, 1, 20));
    editor.setLabel(operation, "Updated label");
    editor.setDetails(operation, "Updated details");
    editor.endEditing(true);

    QCOMPARE(operation->amount(), -50.00);
    QCOMPARE(operation->date(), QDate(2026, 1, 20));
    QCOMPARE(operation->budgetDate(), QDate(2026, 2, 1));
    QCOMPARE(operation->label(), QString("Updated label"));
    QCOMPARE(operation->details(), QString("Updated details"));
    QCOMPARE(undoStack.count(), 1);
    QVERIFY(editor.canUndo());

    editor.undo();
    QCOMPARE(operation->amount(), -42.50);
    QCOMPARE(operation->date(), QDate(2026, 1, 15));
    QCOMPARE(operation->budgetDate(), QDate(2026, 1, 15));
    QCOMPARE(operation->label(), QString("Original label"));
    QCOMPARE(operation->details(), QString("Original details"));
    QVERIFY(editor.canRedo());

    editor.redo();
    QCOMPARE(operation->amount(), -50.00);
    QCOMPARE(operation->budgetDate(), QDate(2026, 2, 1));
  }

  void beginNewRequiresCurrentAccountAndCanBeCancelled() {
    QUndoStack emptyUndoStack;
    BudgetData emptyBudget(emptyUndoStack);
    OperationEditor emptyEditor(emptyBudget, emptyUndoStack);
    QVERIFY(emptyEditor.beginNew(QDate(2026, 1, 1), 10.0, "No account", "") == nullptr);

    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* account = budgetData.createAccount("Fictional Checking");
    budgetData.set_currentAccount(account);
    OperationEditor editor(budgetData, undoStack);

    auto* operation = editor.beginNew(QDate(2026, 2, 1), -20.0, "New operation", "Details");
    QVERIFY(operation != nullptr);
    QCOMPARE(account->operations().size(), 1);
    QVERIFY(editor.isEditing());

    editor.endEditing(false);
    QCOMPARE(account->operations().size(), 0);
    QCOMPARE(undoStack.count(), 1);

    operation = editor.beginNew(QDate(2026, 2, 1), -20.0, "New operation", "Details");
    QVERIFY(operation != nullptr);
    editor.endEditing(true);
    QCOMPARE(account->operations().size(), 1);
    QCOMPARE(undoStack.count(), 1);
  }

  void addFiltersAllocationVariantsAndIsUndoable() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* account = budgetData.createAccount("Fictional Checking");
    budgetData.set_currentAccount(account);
    Category category("Fictional Category");
    auto* allocation = new Allocation(&category, -25.0);
    QVariantList values = { QVariant::fromValue(static_cast<QObject*>(allocation)), 42 };
    OperationEditor editor(budgetData, undoStack);

    editor.add(QDate(2026, 4, 1), -25.0, "Added operation", "Details", values);

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
    auto* account = budgetData.createAccount("Fictional Checking");
    Category food("Fictional Food");
    Category travel("Fictional Travel");
    auto* operation = account->addOperation(
        new Operation(account, QDate(2026, 1, 1), -100.0, "Mixed purchase"));
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
    auto* sourceAccount = budgetData.createAccount("Fictional Checking");
    auto* targetAccount = budgetData.createAccount("Fictional Savings");
    budgetData.set_currentAccount(sourceAccount);
    Category food("Fictional Food");
    Category travel("Fictional Travel");
    auto* operation = sourceAccount->addOperation(
        new Operation(sourceAccount, QDate(2026, 3, 1), -100.0, "Transfer", "Details",
                      { new Allocation(&food, -60.0), new Allocation(&travel, -40.0) }));
    OperationEditor editor(budgetData, undoStack);

    auto* counterpart = editor.createCounterpart(operation, targetAccount, "Fictional Food");
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
    QCOMPARE(sourceAccount->operationAt(0)->label(), QString("Transfer"));
  }
};

QTEST_GUILESS_MAIN(OperationEditorTest)
#include "OperationEditorTest.moc"
