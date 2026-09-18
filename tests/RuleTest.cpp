#include <QTest>
#include <QUndoStack>

#include "editor/RuleEditor.h"
#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "model/Rule.h"
#include "services/BudgetData.h"
#include "services/RuleController.h"

using namespace Qt::StringLiterals;

class RuleTest : public QObject {
  Q_OBJECT

private slots:
  void testMatchesLabelCaseInsensitively() {
    Category category(u"Food"_s);
    Rule rule(&category, u"supermarket"_s);
    Operation operation(nullptr, {}, -42.50, u"ACME SUPERMARKET 123"_s);

    QVERIFY(rule.matches(&operation));
  }

  void testDoesNotMatchMissingOrEmptyLabel() {
    Category category(u"Food"_s);
    Rule emptyRule;
    QVERIFY(!emptyRule.matches(nullptr));
    Rule rule(&category, u"market"_s);
    Operation operation(nullptr, {}, -42.50, u"Coffee Shop"_s);

    QVERIFY(!rule.matches(nullptr));
    QVERIFY(!rule.matches(&operation));

    rule.set_labelMatch(QString());
    QVERIFY(!rule.matches(&operation));
  }

  void testAmountFilterIsOptionalAndExact() {
    Category category(u"Food"_s);
    Rule rule(&category, u"store"_s);
    Operation operation(nullptr, {}, -42.50, u"Corner Store"_s);

    QVERIFY(rule.matches(&operation));

    rule.set_amountFilter(-42.50);
    QVERIFY(rule.matches(&operation));
    rule.set_amountFilter(-42.51);
    QVERIFY(!rule.matches(&operation));
  }

  void testControllerUsesRulePriority() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    RuleEditor editor(controller, budgetData, undoStack);
    Category broad(u"Broad"_s);
    Category specific(u"Specific"_s);
    Operation operation(nullptr, {}, -10.0, u"ACME STORE"_s);

    editor.add(&broad, u"ACME"_s);
    editor.add(&specific, u"STORE"_s);

    QCOMPARE(controller.matchingCategory(&operation), &broad);
  }

  void testControllerAppliesRulesToOperations() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    Category category(u"Fictional Bills"_s);
    controller.addRule(new Rule(&category, u"Utility"_s, -25.0));

    Operation matching(nullptr, {}, -25.0, u"Fictional Utility bill"_s);
    Operation wrongAmount(nullptr, {}, -30.0, u"Fictional Utility bill"_s);
    Operation unmatched(nullptr, {}, -25.0, u"Fictional Grocery"_s);
    QVERIFY(controller.matchingCategory(nullptr) == nullptr);
    QCOMPARE(controller.matchingCategory(&matching), &category);
    QVERIFY(controller.matchingCategory(&wrongAmount) == nullptr);
    QVERIFY(controller.matchingCategory(&unmatched) == nullptr);

    QCOMPARE(controller.applyRulesToOperation(nullptr), 0);
    QCOMPARE(controller.applyRulesToOperation(&unmatched), 0);
    QCOMPARE(controller.applyRulesToOperation(&matching), 1);
    QVERIFY(matching.isCategorized());
    QCOMPARE(controller.applyRulesToOperation(&matching), 0);
  }

  void testApplyRuleCategorizesUncategorizedOperation() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    RuleEditor editor(controller, budgetData, undoStack);
    Category category(u"Bills"_s);
    Account* account = budgetData.createAccount(u"Fictional Checking"_s);
    auto* operation = account->addOperation(
        new Operation(account, {}, -25.0, u"Fictional Utility"_s), false);

    QCOMPARE(editor.applyToUncategorized(&category, "Utility"_L1), 1);
    QVERIFY(operation->isCategorized());
    QCOMPARE(operation->allocations().size(), 1);
    QCOMPARE(operation->allocations().at(0)->category(), &category);
    QCOMPARE(operation->allocations().at(0)->amount(), -25.0);

    QCOMPARE(editor.applyToUncategorized(&category, "Utility"_L1), 0);
  }

  void testAddingRuleCanBeUndone() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    RuleEditor editor(controller, budgetData, undoStack);
    Category category(u"Transport"_s);

    editor.add(&category, u"Fictional Rail"_s);
    QCOMPARE(controller.ruleCount(), 1);
    undoStack.undo();
    QCOMPARE(controller.ruleCount(), 0);
    undoStack.redo();
    QCOMPARE(controller.ruleCount(), 1);
  }

  void testEditingRuleToUniqueMatchSucceeds() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    RuleEditor editor(controller, budgetData, undoStack);
    Category category(u"Transport"_s);

    editor.add(&category, u"Fictional Bus"_s);
    editor.edit(0, &category, u"Fictional Train"_s, -12.50);

    QCOMPARE(controller.at(0)->labelMatch(), u"Fictional Train"_s);
    QCOMPARE(controller.at(0)->amountFilter(), -12.50);
  }

  void testEditingRuleToDuplicateIsRejected() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    RuleEditor editor(controller, budgetData, undoStack);
    Category category(u"Transport"_s);
    Category otherCategory(u"Other"_s);

    editor.add(&category, u"Fictional Bus"_s, -12.50);
    editor.add(&otherCategory, u"Fictional Train"_s, -25.00);
    const int undoCountBeforeDuplicateEdit = undoStack.count();

    editor.edit(0, &category, u"fictional train"_s, -25.00);

    QCOMPARE(controller.at(0)->labelMatch(), u"Fictional Bus"_s);
    QCOMPARE(controller.at(0)->amountFilter(), -12.50);
    QCOMPARE(controller.at(1)->labelMatch(), u"Fictional Train"_s);
    QCOMPARE(undoStack.count(), undoCountBeforeDuplicateEdit);
  }
};

QTEST_GUILESS_MAIN(RuleTest)
#include "RuleTest.moc"
