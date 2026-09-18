#include <QTest>
#include <QUndoStack>

#include "editor/RuleEditor.h"
#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "services/BudgetData.h"
#include "services/RuleController.h"

using namespace Qt::StringLiterals;

class RuleEditorTest : public QObject {
  Q_OBJECT

private slots:
  void managesRulesAndRejectsInvalidChanges() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    RuleEditor editor(controller, budgetData, undoStack);
    Category firstCategory(u"Fictional First"_s);
    Category secondCategory(u"Fictional Second"_s);

    editor.add(nullptr, u"Fictional Match"_s);
    editor.add(&firstCategory, QString());
    QCOMPARE(controller.ruleCount(), 0);

    editor.add(&firstCategory, u"Fictional Match"_s, -10.0);
    editor.add(&firstCategory, u"fictional match"_s, -10.0);
    QCOMPARE(controller.ruleCount(), 1);
    editor.add(&secondCategory, u"Fictional Other"_s, -20.0);
    QCOMPARE(controller.ruleCount(), 2);

    const int commandCount = undoStack.count();
    editor.edit(-1, &firstCategory, u"Invalid"_s);
    editor.edit(0, nullptr, u"Invalid"_s);
    editor.edit(0, &firstCategory, QString());
    editor.edit(0, &firstCategory, u"Fictional Match"_s, -10.0);
    editor.edit(0, &secondCategory, u"Fictional Other"_s, -20.0);
    QCOMPARE(undoStack.count(), commandCount);

    editor.edit(0, &secondCategory, u"Fictional Updated"_s, -30.0);
    QCOMPARE(controller.at(0)->labelMatch(), u"Fictional Updated"_s);
    QCOMPARE(controller.at(0)->amountFilter(), -30.0);

    editor.move(-1, 0);
    editor.move(0, 5);
    editor.move(0, 0);
    editor.move(0, 1);
    QCOMPARE(controller.at(0)->labelMatch(), u"Fictional Other"_s);
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
    Category category(u"Fictional Category"_s);
    auto* account = budgetData.createAccount(u"Fictional Checking"_s);
    auto* matching = account->addOperation(
        new Operation(account, QDate(2026, 2, 1), -15.0, u"Fictional Grocery"_s));
    auto* wrongAmount = account->addOperation(
        new Operation(account, QDate(2026, 2, 2), -20.0, u"Fictional Grocery"_s));
    auto* categorized = account->addOperation(
        new Operation(account, QDate(2026, 2, 3), -15.0, u"Fictional Grocery"_s,
                      { new Allocation(&category, -15.0) }, {}));

    QCOMPARE(editor.applyToUncategorized(nullptr, "Fictional Grocery"_L1, -15.0), 0);
    QCOMPARE(editor.applyToUncategorized(&category, QString(), -15.0), 0);
    QCOMPARE(editor.applyToUncategorized(&category, "Fictional Grocery"_L1, -15.0), 1);
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
