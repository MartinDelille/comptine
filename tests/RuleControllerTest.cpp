#include <QTest>
#include <QUndoStack>

#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "model/Rule.h"
#include "services/BudgetData.h"
#include "services/RuleController.h"

class RuleControllerTest : public QObject {
  Q_OBJECT

private slots:
  void managesRulesAndRejectsInvalidIndexes() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    Category firstCategory("Fictional First");
    Category secondCategory("Fictional Second");

    controller.addRule(nullptr);
    QCOMPARE(controller.ruleCount(), 0);
    controller.addRule(new Rule(&firstCategory, "Fictional First Match"));
    controller.addRule(new Rule(&secondCategory, "Fictional Second Match"));
    QCOMPARE(controller.ruleCount(), 2);
    QVERIFY(controller.at(-1) == nullptr);
    QVERIFY(controller.at(2) == nullptr);

    auto* duplicate = new Rule(&secondCategory, "fictional first match");
    controller.addRule(duplicate);
    QCOMPARE(controller.ruleCount(), 2);
    delete duplicate;

    controller.moveRuleDirect(-1, 0);
    controller.moveRuleDirect(0, 2);
    controller.moveRuleDirect(0, 0);
    QCOMPARE(controller.at(0)->category(), &firstCategory);

    auto* taken = controller.takeRule(0);
    QVERIFY(taken != nullptr);
    QCOMPARE(controller.ruleCount(), 1);
    taken->setParent(nullptr);
    delete taken;
    QVERIFY(controller.takeRule(-1) == nullptr);
    QVERIFY(controller.takeRule(5) == nullptr);

    controller.clearRules();
    QCOMPARE(controller.ruleCount(), 0);
    controller.clearRules();
    QCOMPARE(controller.ruleModel()->rowCount(), 0);
  }

  void ruleModelExposesRuleData() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    Category category("Fictional Category");
    controller.addRule(new Rule(&category, "Fictional Match", -12.50));

    auto* model = controller.ruleModel();
    QCOMPARE(model->rowCount(), 1);
    const QModelIndex index = model->index(0, 0);
    QCOMPARE(model->data(index, RuleListModel::CategoryRole).toString(), QString("Fictional Category"));
    QCOMPARE(model->data(index, RuleListModel::LabelMatchRole).toString(), QString("Fictional Match"));
    QCOMPARE(model->data(index, RuleListModel::AmountFilterRole).toDouble(), -12.50);
    QVERIFY(!model->data(QModelIndex(), RuleListModel::CategoryRole).isValid());
    QVERIFY(!model->data(model->index(5, 0), RuleListModel::CategoryRole).isValid());
  }

  void navigatesUncategorizedOperationsAcrossAccounts() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    Category category("Fictional Category");
    auto* firstAccount = budgetData.createAccount("Fictional Checking");
    auto* secondAccount = budgetData.createAccount("Fictional Savings");
    auto* first = firstAccount->addOperation(
        new Operation(firstAccount, QDate(2026, 1, 1), -10.0, "First"), false);
    auto* categorized = firstAccount->addOperation(
        new Operation(firstAccount, QDate(2026, 1, 2), -20.0, "Categorized",
                      {}, { new Allocation(&category, -20.0) }),
        false);
    auto* second = secondAccount->addOperation(
        new Operation(secondAccount, QDate(2026, 1, 3), -30.0, "Second"), false);

    QCOMPARE(controller.nextUncategorizedOperation(nullptr), first);
    QCOMPARE(controller.nextUncategorizedOperation(first), second);
    QVERIFY(controller.nextUncategorizedOperation(second) == nullptr);
    QCOMPARE(controller.nextUncategorizedOperation(categorized), second);
    QVERIFY(controller.nextUncategorizedOperation(nullptr) != categorized);

    QVERIFY(controller.previousUncategorizedOperation(nullptr) == nullptr);
    QVERIFY(controller.previousUncategorizedOperation(first) == nullptr);
    QCOMPARE(controller.previousUncategorizedOperation(second), first);
    QVERIFY(controller.previousUncategorizedOperation(categorized) == first);
  }
};

QTEST_GUILESS_MAIN(RuleControllerTest)
#include "RuleControllerTest.moc"
