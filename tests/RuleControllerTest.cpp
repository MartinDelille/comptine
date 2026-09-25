#include <QTest>
#include <QUndoStack>

#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "model/Rule.h"
#include "services/BudgetData.h"
#include "services/RuleController.h"

using namespace Qt::StringLiterals;

class RuleControllerTest : public QObject {
  Q_OBJECT

private slots:
  void managesRulesAndRejectsInvalidIndexes() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    Category firstCategory(u"Fictional First"_s);
    Category secondCategory(u"Fictional Second"_s);

    controller.addRule(nullptr);
    QCOMPARE(controller.ruleCount(), 0);
    controller.addRule(new Rule(&firstCategory, u"Fictional First Match"_s));
    controller.addRule(new Rule(&secondCategory, u"Fictional Second Match"_s));
    QCOMPARE(controller.ruleCount(), 2);
    QVERIFY(controller.at(-1) == nullptr);
    QVERIFY(controller.at(2) == nullptr);

    auto* duplicate = new Rule(&secondCategory, u"fictional first match"_s);
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
    Category category(u"Fictional Category"_s);
    controller.addRule(new Rule(&category, u"Fictional Match"_s, -12.50));
    controller.addRule(new Rule(nullptr, u"Fictional Without Category"_s));

    auto* model = controller.ruleModel();
    QCOMPARE(model->rowCount(), 2);
    const QModelIndex index = model->index(0, 0);
    QCOMPARE(model->data(index, RuleListModel::CategoryRole).toString(), u"Fictional Category"_s);
    QCOMPARE(model->data(index, RuleListModel::LabelMatchRole).toString(), u"Fictional Match"_s);
    QCOMPARE(model->data(index, RuleListModel::AmountFilterRole).toDouble(), -12.50);
    QVERIFY(!model->data(model->index(1, 0), RuleListModel::CategoryRole).isValid());
    QVERIFY(!model->data(QModelIndex(), RuleListModel::CategoryRole).isValid());
    QVERIFY(!model->data(model->index(5, 0), RuleListModel::CategoryRole).isValid());

    RuleListModel emptyModel;
    QCOMPARE(emptyModel.rowCount(), 0);
    QCOMPARE(emptyModel.rowCount(model->index(0, 0)), 0);
    QVERIFY(!emptyModel.data(QModelIndex(), RuleListModel::CategoryRole).isValid());
    QVERIFY(!emptyModel.data(model->index(0, 0), RuleListModel::CategoryRole).isValid());
    const auto roles = emptyModel.roleNames();
    QCOMPARE(roles.value(RuleListModel::CategoryRole), "category"_ba);
    QCOMPARE(roles.value(RuleListModel::LabelMatchRole), "labelMatch"_ba);
    QCOMPARE(roles.value(RuleListModel::AmountFilterRole), "amountFilter"_ba);

    model->refresh();
  }

  void navigatesUncategorizedOperationsAcrossAccounts() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    Category category(u"Fictional Category"_s);
    auto* firstAccount = budgetData.createAccount(u"Fictional Checking"_s);
    auto* secondAccount = budgetData.createAccount(u"Fictional Savings"_s);
    auto* first = firstAccount->addOperation(
        new Operation(firstAccount, QDate(2026, 1, 1), -10.0, u"First"_s), false);
    auto* categorized = firstAccount->addOperation(
        new Operation(firstAccount, QDate(2026, 1, 2), -20.0, u"Categorized"_s,
                      { new Allocation(&category, -20.0) }, {}),
        false);
    auto* second = secondAccount->addOperation(
        new Operation(secondAccount, QDate(2026, 1, 3), -30.0, u"Second"_s), false);

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
