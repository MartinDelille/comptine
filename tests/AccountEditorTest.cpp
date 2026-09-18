#include <QTest>
#include <QUndoStack>

#include "editor/AccountEditor.h"
#include "services/BudgetData.h"

using namespace Qt::StringLiterals;

class AccountEditorTest : public QObject {
  Q_OBJECT

private slots:
  void renameWithoutCurrentAccountDoesNothing() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    AccountEditor editor(budgetData, undoStack);

    editor.renameCurrentAccount(u"Fictional Savings"_s);

    QCOMPARE(budgetData.rowCount(), 0);
    QCOMPARE(undoStack.count(), 0);
  }

  void emptyOrSameNameDoesNothing() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* account = budgetData.createAccount(u"Fictional Checking"_s);
    budgetData.set_currentAccount(account);
    AccountEditor editor(budgetData, undoStack);

    editor.renameCurrentAccount(QString());
    editor.renameCurrentAccount(u"Fictional Checking"_s);

    QCOMPARE(account->name(), u"Fictional Checking"_s);
    QCOMPARE(undoStack.count(), 0);
  }

  void renameIsUndoable() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* account = budgetData.createAccount(u"Fictional Checking"_s);
    budgetData.set_currentAccount(account);
    AccountEditor editor(budgetData, undoStack);

    editor.renameCurrentAccount(u"Fictional Savings"_s);

    QCOMPARE(account->name(), u"Fictional Savings"_s);
    QCOMPARE(undoStack.count(), 1);

    undoStack.undo();
    QCOMPARE(account->name(), u"Fictional Checking"_s);

    undoStack.redo();
    QCOMPARE(account->name(), u"Fictional Savings"_s);
  }
};

QTEST_GUILESS_MAIN(AccountEditorTest)
#include "AccountEditorTest.moc"
