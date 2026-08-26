#include <QTest>
#include <QUndoStack>

#include "../BudgetData.h"
#include "../Category.h"
#include "../Operation.h"
#include "../Rule.h"
#include "../RuleController.h"

class RuleTest : public QObject {
  Q_OBJECT

private slots:
  void testMatchesLabelCaseInsensitively() {
    Category category("Food");
    Rule rule(&category, "supermarket");
    Operation operation(nullptr, {}, -42.50, "ACME SUPERMARKET 123");

    QVERIFY(rule.matches(&operation));
  }

  void testDoesNotMatchMissingOrEmptyLabel() {
    Category category("Food");
    Rule rule(&category, "market");
    Operation operation(nullptr, {}, -42.50, "Coffee Shop");

    QVERIFY(!rule.matches(nullptr));
    QVERIFY(!rule.matches(&operation));

    rule.set_labelMatch(QString());
    QVERIFY(!rule.matches(&operation));
  }

  void testAmountFilterIsOptionalAndExact() {
    Category category("Food");
    Rule rule(&category, "store");
    Operation operation(nullptr, {}, -42.50, "Corner Store");

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
    Category broad("Broad");
    Category specific("Specific");
    Operation operation(nullptr, {}, -10.0, "ACME STORE");

    controller.addRule(&broad, "ACME");
    controller.addRule(&specific, "STORE");

    QCOMPARE(controller.matchingCategory(&operation), &broad);
  }

  void testApplyRuleCategorizesUncategorizedOperation() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    Category category("Bills");
    Account* account = budgetData.createAccount("Fictional Checking");
    auto* operation = account->addOperation(
        new Operation(account, {}, -25.0, "Fictional Utility"), false);

    QCOMPARE(controller.applyRuleToUncategorized(&category, "Utility"), 1);
    QVERIFY(operation->isCategorized());
    QCOMPARE(operation->allocations().size(), 1);
    QCOMPARE(operation->allocations().at(0)->category(), &category);
    QCOMPARE(operation->allocations().at(0)->amount(), -25.0);

    QCOMPARE(controller.applyRuleToUncategorized(&category, "Utility"), 0);
  }

  void testAddingRuleCanBeUndone() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    Category category("Transport");

    controller.addRule(&category, "Fictional Rail");
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
    Category category("Transport");

    controller.addRule(&category, "Fictional Bus");
    controller.editRule(0, &category, "Fictional Train", -12.50);

    QCOMPARE(controller.at(0)->labelMatch(), QString("Fictional Train"));
    QCOMPARE(controller.at(0)->amountFilter(), -12.50);
  }

  void testEditingRuleToDuplicateIsRejected() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    Category category("Transport");
    Category otherCategory("Other");

    controller.addRule(&category, "Fictional Bus", -12.50);
    controller.addRule(&otherCategory, "Fictional Train", -25.00);
    const int undoCountBeforeDuplicateEdit = undoStack.count();

    controller.editRule(0, &category, "fictional train", -25.00);

    QCOMPARE(controller.at(0)->labelMatch(), QString("Fictional Bus"));
    QCOMPARE(controller.at(0)->amountFilter(), -12.50);
    QCOMPARE(controller.at(1)->labelMatch(), QString("Fictional Train"));
    QCOMPARE(undoStack.count(), undoCountBeforeDuplicateEdit);
  }
};

QTEST_GUILESS_MAIN(RuleTest)
#include "RuleTest.moc"
