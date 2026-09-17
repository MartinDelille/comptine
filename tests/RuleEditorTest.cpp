#include <QTest>
#include <QUndoStack>

#include "editor/RuleEditor.h"
#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "services/BudgetData.h"
#include "services/RuleController.h"

class RuleEditorTest : public QObject {
  Q_OBJECT

private slots:
  void managesRulesAndRejectsInvalidChanges() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    RuleEditor editor(controller, budgetData, undoStack);
    Category firstCategory("Fictional First");
    Category secondCategory("Fictional Second");

    editor.add(nullptr, "Fictional Match");
    editor.add(&firstCategory, QString());
    QCOMPARE(controller.ruleCount(), 0);

    editor.add(&firstCategory, "Fictional Match", -10.0);
    editor.add(&firstCategory, "fictional match", -10.0);
    QCOMPARE(controller.ruleCount(), 1);
    editor.add(&secondCategory, "Fictional Other", -20.0);
    QCOMPARE(controller.ruleCount(), 2);

    const int commandCount = undoStack.count();
    editor.edit(-1, &firstCategory, "Invalid");
    editor.edit(0, nullptr, "Invalid");
    editor.edit(0, &firstCategory, QString());
    editor.edit(0, &firstCategory, "Fictional Match", -10.0);
    editor.edit(0, &secondCategory, "Fictional Other", -20.0);
    QCOMPARE(undoStack.count(), commandCount);

    editor.edit(0, &secondCategory, "Fictional Updated", -30.0);
    QCOMPARE(controller.at(0)->labelMatch(), QString("Fictional Updated"));
    QCOMPARE(controller.at(0)->amountFilter(), -30.0);

    editor.move(-1, 0);
    editor.move(0, 5);
    editor.move(0, 0);
    editor.move(0, 1);
    QCOMPARE(controller.at(0)->labelMatch(), QString("Fictional Other"));
    editor.remove(-1);
    editor.remove(5);
    QCOMPARE(controller.ruleCount(), 2);
    editor.remove(0);
    QCOMPARE(controller.ruleCount(), 1);
    undoStack.undo();
    QCOMPARE(controller.ruleCount(), 2);
  }

  void appliesRulesOnlyToMatchingUncategorizedOperations() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    RuleEditor editor(controller, budgetData, undoStack);
    Category category("Fictional Category");
    auto* account = budgetData.createAccount("Fictional Checking");
    auto* matching = account->addOperation(
        new Operation(account, QDate(2026, 2, 1), -15.0, "Fictional Grocery"));
    auto* wrongAmount = account->addOperation(
        new Operation(account, QDate(2026, 2, 2), -20.0, "Fictional Grocery"));
    auto* categorized = account->addOperation(
        new Operation(account, QDate(2026, 2, 3), -15.0, "Fictional Grocery",
                      {}, { new Allocation(&category, -15.0) }));

    QCOMPARE(editor.applyToUncategorized(nullptr, "Fictional Grocery", -15.0), 0);
    QCOMPARE(editor.applyToUncategorized(&category, QString(), -15.0), 0);
    QCOMPARE(editor.applyToUncategorized(&category, "Fictional Grocery", -15.0), 1);
    QVERIFY(matching->isCategorized());
    QVERIFY(!wrongAmount->isCategorized());
    QVERIFY(categorized->isCategorized());
    QCOMPARE(undoStack.count(), 1);

    undoStack.undo();
    QVERIFY(!matching->isCategorized());
    QCOMPARE(wrongAmount->allocations().size(), 0);
    QVERIFY(categorized->isCategorized());
  }
};

QTEST_GUILESS_MAIN(RuleEditorTest)
#include "RuleEditorTest.moc"
