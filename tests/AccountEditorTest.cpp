#include <QTest>
#include <QUndoStack>

#include "editor/AccountEditor.h"
#include "services/BudgetData.h"

class AccountEditorTest : public QObject {
  Q_OBJECT

private slots:
  void renameWithoutCurrentAccountDoesNothing() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    AccountEditor editor(budgetData, undoStack);

    editor.renameCurrentAccount("Fictional Savings");

    QCOMPARE(budgetData.rowCount(), 0);
    QCOMPARE(undoStack.count(), 0);
  }

  void emptyOrSameNameDoesNothing() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* account = budgetData.createAccount("Fictional Checking");
    budgetData.set_currentAccount(account);
    AccountEditor editor(budgetData, undoStack);

    editor.renameCurrentAccount(QString());
    editor.renameCurrentAccount("Fictional Checking");

    QCOMPARE(account->name(), QString("Fictional Checking"));
    QCOMPARE(undoStack.count(), 0);
  }

  void renameIsUndoable() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* account = budgetData.createAccount("Fictional Checking");
    budgetData.set_currentAccount(account);
    AccountEditor editor(budgetData, undoStack);

    editor.renameCurrentAccount("Fictional Savings");

    QCOMPARE(account->name(), QString("Fictional Savings"));
    QCOMPARE(undoStack.count(), 1);

    undoStack.undo();
    QCOMPARE(account->name(), QString("Fictional Checking"));

    undoStack.redo();
    QCOMPARE(account->name(), QString("Fictional Savings"));
  }
};

QTEST_GUILESS_MAIN(AccountEditorTest)
#include "AccountEditorTest.moc"
