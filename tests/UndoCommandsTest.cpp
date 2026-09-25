#include <QDate>
#include <QTest>
#include <QUndoStack>

#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "model/Rule.h"
#include "services/BudgetData.h"
#include "services/CategoryController.h"
#include "services/RuleController.h"
#include "services/UndoCommands.h"

using namespace Qt::StringLiterals;

class UndoCommandsTest : public QObject {
  Q_OBJECT

private slots:
  void accountAndImportCommandsRoundTrip() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    auto* account = new Account(u"Fictional Checking"_s, nullptr);
    undoStack.push(new AddAccountCommand(account, budgetData));
    QCOMPARE(budgetData.rowCount(), 1);
    QCOMPARE(budgetData.currentAccount(), account);

    undoStack.undo();
    QCOMPARE(budgetData.rowCount(), 0);
    undoStack.redo();
    QCOMPARE(budgetData.rowCount(), 1);

    auto* first = new Operation(account, QDate(2026, 1, 1), -10.0, u"Fictional First"_s);
    auto* second = new Operation(account, QDate(2026, 1, 2), -20.0, u"Fictional Second"_s);
    undoStack.push(new ImportOperationsCommand(*account, { first, second }));
    QCOMPARE(account->operations().size(), 2);
    undoStack.undo();
    QCOMPARE(account->operations().size(), 0);
    undoStack.redo();
    QCOMPARE(account->operations().size(), 2);
  }

  void addOperationCommandIsClearedBeforeAccountDestruction() {
    QUndoStack undoStack;
    {
      BudgetData budgetData(undoStack);
      auto* account = budgetData.createAccount(u"Fictional Checking"_s);
      auto* operation = new Operation(account, QDate(2026, 1, 1), -10.0,
                                      u"Fictional Operation"_s);
      undoStack.push(new AddOperationCommand(operation, *account));
      QCOMPARE(account->operations().size(), 1);
    }

    QVERIFY(true);
  }

  void categoryCommandRestoresHistoricalBudgetLimit() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    Category category(u"Fictional Original"_s);
    category.setBudgetLimitForMonth(QDate(2026, 1, 1), -100.0);
    category.setBudgetLimitForMonth(QDate(2026, 2, 1), -80.0);

    undoStack.push(new EditCategoryCommand(category, u"Fictional Updated"_s, -120.0,
                                           QDate(2026, 3, 1)));
    QCOMPARE(category.name(), u"Fictional Updated"_s);
    QCOMPARE(category.budgetLimitForMonth(QDate(2026, 3, 1)), -120.0);
    QCOMPARE(category.budgetLimitForMonth(QDate(2026, 2, 1)), -80.0);

    undoStack.undo();
    QCOMPARE(category.name(), u"Fictional Original"_s);
    QCOMPARE(category.budgetLimitForMonth(QDate(2026, 2, 1)), -80.0);
    undoStack.redo();
    QCOMPARE(category.budgetLimitForMonth(QDate(2026, 3, 1)), -120.0);
  }

  void ruleCommandsRoundTrip() {
    QUndoStack undoStack;
    BudgetData budgetData(undoStack);
    RuleController controller(budgetData, undoStack);
    Category firstCategory(u"Fictional First"_s);
    Category secondCategory(u"Fictional Second"_s);
    auto* firstRule = new Rule(&firstCategory, u"Fictional First"_s);
    auto* secondRule = new Rule(&secondCategory, u"Fictional Second"_s);

    undoStack.push(new AddRuleCommand(&controller, firstRule));
    undoStack.push(new AddRuleCommand(&controller, secondRule));
    QCOMPARE(controller.ruleCount(), 2);

    undoStack.push(new EditRuleCommand(controller, firstRule, &secondCategory,
                                       u"Fictional Updated"_s, -15.0));
    QCOMPARE(controller.at(0)->category(), &secondCategory);
    QCOMPARE(controller.at(0)->labelMatch(), u"Fictional Updated"_s);
    undoStack.undo();
    QCOMPARE(controller.at(0)->category(), &firstCategory);
    undoStack.redo();

    undoStack.push(new MoveRuleCommand(controller, { 0, 1 }));
    QCOMPARE(controller.at(0), secondRule);
    undoStack.undo();
    QCOMPARE(controller.at(0), firstRule);
    undoStack.redo();

    undoStack.push(new RemoveRuleCommand(&controller, 0));
    QCOMPARE(controller.ruleCount(), 1);
    undoStack.undo();
    QCOMPARE(controller.ruleCount(), 2);
    undoStack.redo();
    QCOMPARE(controller.ruleCount(), 1);

    // Release command-owned removed rules before the controller is destroyed.
    undoStack.clear();
  }
};

QTEST_GUILESS_MAIN(UndoCommandsTest)
#include "UndoCommandsTest.moc"
