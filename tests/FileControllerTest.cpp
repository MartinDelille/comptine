// Integration tests for FileController
#include <QDate>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include "editor/CategoryEditor.h"
#include "editor/ImportEditor.h"
#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "model/Rule.h"
#include "services/AppSettings.h"
#include "services/BudgetData.h"
#include "services/CategoryController.h"

using namespace Qt::StringLiterals;
#include "services/FileController.h"
#include "services/RuleController.h"
#include "services/UndoCommands.h"

Q_DECLARE_METATYPE(QDate)

class FileControllerTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase() {
    // Create temporary directory for test files
    tempDir = new QTemporaryDir();
    QVERIFY(tempDir->isValid());
  }

  void init() {
    // Create fresh instances before each test
    undoStack = new QUndoStack();  // No parent - we'll delete manually
    budgetData = new BudgetData(*undoStack);
    appSettings = new AppSettings();
    categoryController = new CategoryController(*budgetData, *undoStack);
    categoryEditor = new CategoryEditor(*categoryController, *budgetData, *undoStack);
    ruleController = new RuleController(*budgetData, *undoStack);
    fileController = new FileController(*appSettings, *budgetData, *categoryController, *ruleController, *undoStack);
  }

  void cleanup() {
    delete fileController;
    delete categoryEditor;
    delete ruleController;
    delete categoryController;
    delete budgetData;
    delete appSettings;
    delete undoStack;
  }

  void cleanupTestCase() { delete tempDir; }

  // Basic Operations

  void testABasicSetup() {
    // Just verify objects were created
    QVERIFY(budgetData != nullptr);
    QVERIFY(categoryController != nullptr);
    QVERIFY(fileController != nullptr);
  }

  void testClear() {
    // First just check we can access things
    QCOMPARE(budgetData->rowCount(), 0);
    QCOMPARE(categoryController->rowCount(), 0);

    // Try adding an account
    budgetData->createAccount(u"Test Account"_s);
    QCOMPARE(budgetData->rowCount(), 1);

    // Now try clear
    fileController->clear();

    QCOMPARE(budgetData->rowCount(), 0);
    QCOMPARE(categoryController->rowCount(), 0);
    QCOMPARE(fileController->currentFilePath(), QString());
  }

  void testHasUnsavedChanges() {
    QVERIFY(!fileController->hasUnsavedChanges());

    // Make a change - use undo stack to mark as dirty
    undoStack->push(new QUndoCommand(u"Test change"_s));
    QVERIFY(fileController->hasUnsavedChanges());

    // Save should clear unsaved flag
    QString filePath = tempDir->filePath(u"saved.comptine"_s);
    fileController->saveToYamlFile(filePath);
    QVERIFY(!fileController->hasUnsavedChanges());
  }

  //  Save/Load Empty Files

  void testSaveAndLoadEmptyFile() {
    QString filePath = tempDir->filePath(u"empty.comptine"_s);

    // Save empty budget
    QVERIFY(fileController->saveToYamlFile(filePath));
    QVERIFY(QFile::exists(filePath));
    QCOMPARE(fileController->currentFilePath(), filePath);

    // Load it back
    fileController->clear();
    QVERIFY(fileController->loadFromYamlFile(filePath));

    QCOMPARE(budgetData->rowCount(), 0);
    QCOMPARE(categoryController->rowCount(), 0);
  }

  void testSaveToYamlUrl() {
    QString filePath = tempDir->filePath(u"url_test.comptine"_s);
    QUrl fileUrl = QUrl::fromLocalFile(filePath);

    // Save using QUrl
    QVERIFY(fileController->saveToYamlUrl(fileUrl));
    QVERIFY(QFile::exists(filePath));
  }

  void testSaveToInvalidUrl() {
    QVERIFY(!fileController->saveToYamlUrl(QUrl("https://example.com/file.comptine"_L1)));
    QVERIFY(fileController->errorMessage().contains(u"Invalid or unsupported"_s));
  }

  void testLoadFromYamlUrl() {
    // Create a test file
    QString filePath = tempDir->filePath(u"url_load.comptine"_s);
    budgetData->createAccount(u"Test Account"_s);
    fileController->saveToYamlFile(filePath);

    // Load using QUrl
    fileController->clear();
    QUrl fileUrl = QUrl::fromLocalFile(filePath);
    QVERIFY(fileController->loadFromYamlUrl(fileUrl));
    QCOMPARE(budgetData->rowCount(), 1);
  }

  void testImportEditorDelegatesToFileController() {
    ImportEditor importEditor(*fileController);

    QVERIFY(importEditor.importCsv(QUrl(u"file::/tests/import1.csv"_s),
                                   u"Fictional Imported Account"_s, false));
    QCOMPARE(budgetData->accountByName(u"Fictional Imported Account"_s) != nullptr, true);
  }

  // Save/Load with Accounts and Operations

  void testSaveAndLoadWithSingleAccount() {
    // Create test data
    auto account = budgetData->createAccount(u"Checking Account"_s);
    auto food = categoryController->addCategory(new Category(u"Food"_s));
    account->addOperation(
        new Operation(account,
                      QDate(2025, 1, 15),
                      -50.0,
                      u"Grocery Store"_s,
                      { new Allocation(food, -50) },
                      {}),
        false);

    categoryEditor->edit(u"Food"_s, 200.0);

    // Save to file
    QString filePath = tempDir->filePath(u"single_account.comptine"_s);
    QVERIFY(fileController->saveToYamlFile(filePath));

    // Clear and reload
    fileController->clear();
    QVERIFY(fileController->loadFromYamlFile(filePath));

    // Verify data was restored
    QCOMPARE(budgetData->rowCount(), 1);
    QCOMPARE(categoryController->rowCount(), 1);

    auto loadedAccount = budgetData->at(0);
    QCOMPARE(loadedAccount->name(), u"Checking Account"_s);
    QCOMPARE(loadedAccount->operations().size(), 1);

    auto loadedOp = loadedAccount->operations()[0];
    QCOMPARE(loadedOp->date(), QDate(2025, 1, 15));
    QCOMPARE(loadedOp->amount(), -50.0);
    QCOMPARE(loadedOp->label(), u"Grocery Store"_s);
    QCOMPARE(loadedOp->allocations().count(), 1);
    auto alloc = loadedOp->allocations().at(0);
    QCOMPARE(alloc->category()->name(), u"Food"_s);
    QCOMPARE(alloc->amount(), -50.0);
  }

  void testSaveAndLoadWithMultipleAccounts() {
    // Create multiple accounts
    auto checking = budgetData->createAccount(u"Checking"_s);
    auto savings = budgetData->createAccount(u"Savings"_s);

    // Add operations to each
    checking->addOperation(
        new Operation(checking,
                      QDate(2025, 1, 10), -100, u"Purchase 1"_s),
        false);

    savings->addOperation(new Operation(savings, QDate(2025, 1, 20), 500, u"Deposit"_s), false);

    // Save and reload
    QString filePath = tempDir->filePath(u"multiple_accounts.comptine"_s);
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    // Verify
    QCOMPARE(budgetData->rowCount(), 2);
    QCOMPARE(budgetData->at(0)->name(), u"Checking"_s);
    QCOMPARE(budgetData->at(1)->name(), u"Savings"_s);
    QCOMPARE(budgetData->at(0)->operations().size(), 1);
    QCOMPARE(budgetData->at(1)->operations().size(), 1);
  }

  // Save/Load with Split Operations

  void testSaveAndLoadSplitOperation() {
    auto account = budgetData->createAccount(u"Test Account"_s);

    // Create split operation

    auto food = categoryEditor->edit(u"Food"_s, 200.0);
    auto transport = categoryEditor->edit(u"Transport"_s, 100.0);

    auto op = account->addOperation(
        new Operation(account,
                      QDate(2025, 2, 1),
                      -150.0,
                      u"Mixed Purchase"_s),
        false);
    QList<Allocation*> allocations;
    allocations.append(new Allocation(food, -100.0));
    allocations.append(new Allocation(transport, -50.0));
    op->setAllocations(allocations);

    // Save and reload
    QString filePath = tempDir->filePath(u"split_operation.comptine"_s);
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    // Verify split operation
    auto loadedAccount = budgetData->at(0);
    auto loadedOp = loadedAccount->operations()[0];
    QVERIFY(loadedOp->isCategorized());

    auto loadedAllocs = loadedOp->allocations();
    QCOMPARE(loadedAllocs.size(), 2);
    QCOMPARE(loadedAllocs[0]->category()->name(), u"Food"_s);
    QCOMPARE(loadedAllocs[0]->amount(), -100.0);
    QCOMPARE(loadedAllocs[1]->category()->name(), u"Transport"_s);
    QCOMPARE(loadedAllocs[1]->amount(), -50.0);
  }

  // Save/Load with Budget Dates

  void testSaveAndLoadWithBudgetDate() {
    auto account = budgetData->createAccount(u"Test Account"_s);
    auto op = account->addOperation(new Operation(
                                        account,
                                        QDate(2025, 1, 31),
                                        -75.0,
                                        u"Late Month Purchase"_s),
                                    false);
    auto shopping = categoryController->addCategory(new Category(u"Shopping"_s));
    op->setAllocations({ new Allocation(shopping, -75.0) });
    op->set_budgetDate(QDate(2025, 2, 1));  // Budget to next month

    categoryEditor->edit(u"Shopping"_s, 150.0);

    // Save and reload
    QString filePath = tempDir->filePath(u"budget_date.comptine"_s);
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    // Verify budget date is preserved
    auto loadedOp = budgetData->at(0)->operations()[0];
    QCOMPARE(loadedOp->date(), QDate(2025, 1, 31));
    QCOMPARE(loadedOp->budgetDate(), QDate(2025, 2, 1));
  }

  void testBudgetDateNotSavedWhenSameAsDate() {
    auto account = budgetData->createAccount(u"Test Account"_s);
    auto food = categoryEditor->edit(u"Food"_s, 200.0);

    // budgetDate defaults to date, so it should not be saved
    auto op = account->addOperation(new Operation(
                                        account,
                                        QDate(2025, 3, 15),
                                        -30.0,
                                        u"Normal Purchase"_s),
                                    false);
    op->setAllocations({ new Allocation(food, -30.0) });

    // Save and check file content doesn't have budget_date
    QString filePath = tempDir->filePath(u"no_budget_date.comptine"_s);
    fileController->saveToYamlFile(filePath);

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = file.readAll();
    file.close();

    // budget_date should not appear in the file
    QVERIFY(!content.contains(u"budget_date"_s));
  }

  // Save/Load with Categories and Budget Limits

  void testSaveAndLoadCategories() {
    auto* foodToSave = categoryController->addCategory(new Category(u"Food"_s));
    auto* transportToSave = categoryController->addCategory(new Category(u"Transport"_s));
    auto* entertainmentToSave = categoryController->addCategory(new Category(u"Entertainment"_s));
    foodToSave->setBudgetLimitForMonth(QDate(2025, 1, 1), 500.0);
    transportToSave->setBudgetLimitForMonth(QDate(2025, 1, 1), 200.0);
    entertainmentToSave->setBudgetLimitForMonth(QDate(2025, 1, 1), 100.0);

    // Save and reload
    QString filePath = tempDir->filePath(u"categories.comptine"_s);
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    // Verify categories
    QCOMPARE(categoryController->rowCount(), 3);
    auto food = categoryController->getCategoryByName(u"Food"_s);
    QVERIFY(food != nullptr);
    QCOMPARE(food->budgetLimitForMonth(QDate(2025, 1, 1)), 500.0);
  }

  // Save/Load with Leftover Decisions

  void testSaveAndLoadLeftoverDecisions() {
    auto cat = new Category(u"Savings"_s);
    categoryController->addCategory(cat);

    // Set leftover decision for January 2025
    cat->setLeftoverDecision(QDate(2025, 1, 1), { 100.0, 50.0 });  // save 100, report 50

    // Save and reload
    QString filePath = tempDir->filePath(u"leftover.comptine"_s);
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    // Verify leftover decision
    auto loadedCat = categoryController->getCategoryByName(u"Savings"_s);
    QVERIFY(loadedCat != nullptr);
    LeftoverDecision decision = loadedCat->leftoverDecision(QDate(2025, 1, 1));
    QCOMPARE(decision.saveAmount, 100.0);
    QCOMPARE(decision.reportAmount, 50.0);
  }

  void testLoadLegacyLeftoverFormat() {
    // Create a file with legacy leftover format (action + amount)
    QString filePath = tempDir->filePath(u"legacy_leftover.comptine"_s);
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << u"categories:\n"_s;
    out << u"  - name: Food\n"_s;
    out << u"    budget_limit: 200.00\n"_s;
    out << u"    leftover_decisions:\n"_s;
    out << u"      - year: 2025\n"_s;
    out << u"        month: 1\n"_s;
    out << u"        action: save\n"_s;
    out << u"        amount: 50.00\n"_s;
    out << u"accounts: []\n"_s;
    file.close();

    // Load and verify conversion
    fileController->loadFromYamlFile(filePath);
    auto cat = categoryController->getCategoryByName(u"Food"_s);
    QVERIFY(cat != nullptr);
    LeftoverDecision decision = cat->leftoverDecision(QDate(2025, 1, 1));
    QCOMPARE(decision.saveAmount, 50.0);
    QCOMPARE(decision.reportAmount, 0.0);
  }

  // Save/Load with Month History and Budget Limit Overrides

  void testSaveAndLoadMonthHistoryWithBudgetLimit() {
    auto cat = new Category(u"Groceries"_s);
    categoryController->addCategory(cat);

    // Record the limit effective from June.
    cat->setBudgetLimitForMonth(QDate(2025, 6, 1), -250.0);

    // Also set leftover decision for June
    cat->setLeftoverDecision(QDate(2025, 6, 1), { 30.0, 20.0 });

    // Save and reload
    QString filePath = tempDir->filePath(u"month_history_budget.comptine"_s);
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    // Verify
    auto loaded = categoryController->getCategoryByName(u"Groceries"_s);
    QVERIFY(loaded != nullptr);

    // Verify month record has both leftover data and budget limit
    MonthRecord record = loaded->monthRecord(QDate(2025, 6, 1));
    QCOMPARE(record.saveAmount, 30.0);
    QCOMPARE(record.reportAmount, 20.0);
    QVERIFY(record.budgetLimit.has_value());
    QCOMPARE(record.budgetLimit.value_or(0.0), -250.0);

    // Verify budgetLimitForMonth lookup works after reload
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 3, 1)), 0.0);
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 6, 1)), -250.0);
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 7, 1)), -250.0);
  }

  void testSaveAndLoadMonthHistoryBudgetLimitOnly() {
    auto cat = new Category(u"Transport"_s);
    categoryController->addCategory(cat);

    // Only budget limit in history, no leftover data
    cat->setBudgetLimitForMonth(QDate(2025, 3, 1), -100.0);

    QString filePath = tempDir->filePath(u"budget_limit_only.comptine"_s);
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    auto loaded = categoryController->getCategoryByName(u"Transport"_s);
    QVERIFY(loaded != nullptr);

    MonthRecord record = loaded->monthRecord(QDate(2025, 3, 1));
    QCOMPARE(record.saveAmount, 0.0);
    QCOMPARE(record.reportAmount, 0.0);
    QVERIFY(record.budgetLimit.has_value());
    QCOMPARE(record.budgetLimit.value_or(0.0), -100.0);
  }

  void testLoadLegacyBudgetLimitHistory() {
    const QString filePath = tempDir->filePath(u"legacy_budget_limit_history.comptine"_s);
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << u"categories:\n"_s;
    out << u"  - name: Food\n"_s;
    out << u"    budget_limit: -300.00\n"_s;
    out << u"    month_history:\n"_s;
    out << u"      - year: 2025\n"_s;
    out << u"        month: 6\n"_s;
    out << u"        budget_limit: -250.00\n"_s;
    out << u"      - year: 2025\n"_s;
    out << u"        month: 8\n"_s;
    out << u"        budget_limit: -200.00\n"_s;
    out << u"accounts: []\n"_s;
    file.close();

    QVERIFY(fileController->loadFromYamlFile(filePath));
    auto loaded = categoryController->getCategoryByName(u"Food"_s);
    QVERIFY(loaded != nullptr);

    // Legacy entries are converted to month-effective boundaries.
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 5, 1)), 0.0);
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 6, 1)), -250.0);
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 7, 1)), -200.0);
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 8, 1)), -200.0);
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 9, 1)), -300.0);
  }

  void testSaveAndLoadMultipleBudgetLimitChanges() {
    auto cat = new Category(u"Food"_s);
    categoryController->addCategory(cat);

    // Multiple historical budget limit changes
    cat->setBudgetLimitForMonth(QDate(2025, 3, 1), -200.0);  // Was 200 until March
    cat->setBudgetLimitForMonth(QDate(2025, 6, 1), -300.0);  // Was 300 until June
    // Current is 400

    QString filePath = tempDir->filePath(u"multi_budget_limit.comptine"_s);
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    auto loaded = categoryController->getCategoryByName(u"Food"_s);
    QVERIFY(loaded != nullptr);

    // Verify the month-effective lookup works correctly
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 1, 1)), 0.0);
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 3, 1)), -200.0);
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 4, 1)), -200.0);
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 6, 1)), -300.0);
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 7, 1)), -300.0);
  }

  void testLoadLegacyLeftoverDecisionsKey() {
    // Old files use "leftover_decisions"_L1 key — verify it still loads correctly
    QString filePath = tempDir->filePath(u"legacy_key.comptine"_s);
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << u"categories:\n"_s;
    out << u"  - name: Shopping\n"_s;
    out << u"    budget_limit: -200.00\n"_s;
    out << u"    leftover_decisions:\n"_s;
    out << u"      - year: 2025\n"_s;
    out << u"        month: 1\n"_s;
    out << u"        save_amount: 40.00\n"_s;
    out << u"        report_amount: 15.00\n"_s;
    out << u"accounts: []\n"_s;
    file.close();

    fileController->loadFromYamlFile(filePath);
    auto cat = categoryController->getCategoryByName(u"Shopping"_s);
    QVERIFY(cat != nullptr);

    LeftoverDecision decision = cat->leftoverDecision(QDate(2025, 1, 1));
    QCOMPARE(decision.saveAmount, 40.0);
    QCOMPARE(decision.reportAmount, 15.0);
  }

  void testSaveUsesMonthHistoryKey() {
    // Verify that saving uses the new "month_history"_L1 key
    auto cat = new Category(u"Test"_s);
    categoryController->addCategory(cat);
    cat->setLeftoverDecision(QDate(2025, 1, 1), { 10.0, 5.0 });

    QString filePath = tempDir->filePath(u"key_check.comptine"_s);
    fileController->saveToYamlFile(filePath);

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = file.readAll();
    file.close();

    QVERIFY(content.contains(u"month_history"_s));
    QVERIFY(!content.contains(u"leftover_decisions"_s));
    QVERIFY(content.contains(u"budget_limit_history_version: 2"_s));
  }

  // Save/Load with Categorization Rules

  void testSaveAndLoadCategorizationRules() {
    auto groceries = categoryEditor->edit(u"Groceries"_s, 300.0);
    auto fuel = categoryEditor->edit(u"Fuel"_s, 150.0);

    ruleController->addRule(new Rule(groceries, u"SUPERMARKET"_s));
    ruleController->addRule(new Rule(fuel, u"PETROL"_s));

    // Save and reload
    QString filePath = tempDir->filePath(u"rules.comptine"_s);
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    // Verify rules
    QList<Rule*> rules = ruleController->rules();
    QCOMPARE(rules.size(), 2);
    QCOMPARE(rules[0]->category()->name(), u"Groceries"_s);
    QCOMPARE(rules[0]->labelMatch(), u"SUPERMARKET"_s);
  }

  // Error Handling

  void testSaveToInvalidPath() {
    // Keep the failure deterministic without relying on a system path that
    // could exist on a particular machine.
    QString filePath = tempDir->filePath(u"missing-directory/file.comptine"_s);
    QVERIFY(!fileController->saveToYamlFile(filePath));
    QVERIFY(!fileController->errorMessage().isEmpty());
  }

  void testLoadFromNonexistentFile() {
    QString filePath = tempDir->filePath(u"does_not_exist.comptine"_s);
    QVERIFY(!fileController->loadFromYamlFile(filePath));
    QVERIFY(!fileController->errorMessage().isEmpty());
  }

  void testLoadFromEmptyFile() {
    // Create empty file
    QString filePath = tempDir->filePath(u"empty_file.comptine"_s);
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    QVERIFY(!fileController->loadFromYamlFile(filePath));
    QVERIFY(fileController->errorMessage().contains(u"empty"_s));
  }

  // NOTE: Commented out because yaml-cpp throws on truly invalid YAML
  // In production, FileCoordinator validation should catch these earlier
  /*
  void testLoadFromInvalidYaml() {
    // Create file with truly invalid YAML (unclosed bracket)
    QString filePath = tempDir->filePath(u"invalid.comptine"_s);
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << u"categories:\n"_s;
    out << u"  - name: Test\n"_s;
    out << "    budget_limit: [broken yaml\n"_L1;  // Invalid - unclosed bracket
    file.close();

    // Should handle parsing error gracefully
    bool loaded = fileController->loadFromYamlFile(filePath);
    // yaml-cpp may be lenient, so either it fails or succeeds but with error logged
    // We just verify it doesn't crash
    QVERIFY(loaded == loaded);  // Always pass - just checking no crash
  }
  */

  void testLoadFromInvalidUrl() {
    QUrl invalidUrl(u"http://example.com/file.comptine"_s);
    QVERIFY(!fileController->loadFromYamlUrl(invalidUrl));
  }

  void testLoadFromInvalidYaml() {
    QString filePath = tempDir->filePath(u"invalid.comptine"_s);
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("categories: [broken yaml\n"_ba);
    file.close();

    QVERIFY(!fileController->loadFromYamlFile(filePath));
    QVERIFY(fileController->errorMessage().contains(u"Could not parse file"_s));
  }

  void testLoadLegacyStateFields() {
    QString filePath = tempDir->filePath(u"legacy_state.comptine"_s);
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << u"state:\n"_s
        << u"  currentTab: 2\n"_s
        << u"  budgetYear: 2024\n"_s
        << u"  budgetMonth: 11\n"_s
        << u"categories: []\n"_s
        << u"accounts: []\n"_s;
    file.close();

    QVERIFY(fileController->loadFromYamlFile(filePath));
    QCOMPARE(budgetData->currentTabIndex(), 2);
    QCOMPARE(budgetData->budgetDate(), QDate(2024, 11, 1));
  }

  void testSaveAllOptionalFields() {
    auto category = categoryController->addCategory(new Category(u"Food"_s));
    categoryController->set_current(category);
    auto account = budgetData->createAccount(u"Checking"_s);
    budgetData->set_currentAccount(account);
    account->addImportSourcePrefix(u"fictional-bank.csv"_s);

    auto operation = account->addOperation(new Operation(account, QDate(2025, 1, 2), -12.5, u"Lunch"_s), false);
    operation->setAllocations({ new Allocation(nullptr, -12.5) });
    account->select(operation);
    ruleController->addRule(new Rule(category, u"Lunch"_s, -12.5));

    const QString filePath = tempDir->filePath(u"optional_fields.comptine"_s);
    QVERIFY(fileController->saveToYamlFile(filePath));

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString content = file.readAll();
    QVERIFY(content.contains(u"current: true"_s));
    QVERIFY(content.contains(u"import_source_prefixes"_s));
    QVERIFY(content.contains(u"label_match: Lunch"_s));
    QVERIFY(content.contains(u"amount: -12.5"_s));
    QVERIFY(content.contains(u"allocations:"_s));
  }

  void testReloadCurrentFile() {
    auto account = budgetData->createAccount(u"Before"_s);
    account->addOperation(new Operation(account, QDate(2025, 1, 1), 10.0, u"Original"_s), false);
    const QString filePath = tempDir->filePath(u"reload.comptine"_s);
    QVERIFY(fileController->saveToYamlFile(filePath));

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("categories: []\naccounts:\n  - name: After\n    operations: []\n"_ba);
    file.close();

    fileController->reloadCurrentFile();
    QCOMPARE(budgetData->at(0)->name(), u"After"_s);
  }

  void testLoadInitialFileDispatchesByExtension() {
    const QString yamlPath = tempDir->filePath(u"initial.yaml"_s);
    QFile yamlFile(yamlPath);
    QVERIFY(yamlFile.open(QIODevice::WriteOnly | QIODevice::Text));
    yamlFile.write("categories: []\naccounts:\n  - name: YAML\n"_ba);
    yamlFile.close();

    fileController->loadInitialFile({ u"comptine"_s, yamlPath });
    QCOMPARE(budgetData->at(0)->name(), u"YAML"_s);

    fileController->clear();
    const QString csvPath = tempDir->filePath(u"initial.csv"_s);
    QFile csvFile(csvPath);
    QVERIFY(csvFile.open(QIODevice::WriteOnly | QIODevice::Text));
    csvFile.write("Date,Montant,Opération\n01/02/2025,4.50,Initial CSV\n"_ba);
    csvFile.close();

    fileController->loadInitialFile({ u"comptine"_s, csvPath });
    QCOMPARE(budgetData->at(0)->operations().size(), 1);
    QCOMPARE(budgetData->at(0)->operations().first()->label(), u"Initial CSV"_s);
  }

  void testLoadInitialFileUsesExistingRecentFile() {
    const QString filePath = tempDir->filePath(u"recent.comptine"_s);
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("categories: []\naccounts:\n  - name: Recent\n"_ba);
    file.close();
    appSettings->addRecentFile(filePath);

    fileController->loadInitialFile({ u"comptine"_s });
    QCOMPARE(budgetData->at(0)->name(), u"Recent"_s);
  }

  void testImportCsvSkipsInvalidRowsAndDuplicates() {
    auto account = budgetData->createAccount(u"Checking"_s);
    account->addOperation(new Operation(account, QDate(2025, 2, 1), -5.0, u"Duplicate"_s), false);

    const QString csvPath = tempDir->filePath(u"skipped_rows.csv"_s);
    QFile file(csvPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << u"Date;Débit;Crédit;Libellé;budget date\n"_s
        << u"bad-date;-1.00;;Bad date;\n"_s
        << u"01/02/2025;-5.00;;Duplicate;\n"_s
        << u"02/02/2025;;7.50;Credit;03/02/2025\n"_s
        << u"03/02/2025;-2.00;;;\n"_s
        << u"\n"_s;
    file.close();

    QVERIFY(fileController->importFromCsv(QUrl::fromLocalFile(csvPath), u"Checking"_s, false));
    QCOMPARE(account->operations().size(), 2);
    auto imported = account->operationAt(0);
    QCOMPARE(imported->amount(), 7.5);
    QCOMPARE(imported->budgetDate(), QDate(2025, 2, 3));
  }

  void testImportCsvRejectsInvalidHeaderAndEmptyData() {
    const QString invalidPath = tempDir->filePath(u"invalid.csv"_s);
    QFile invalidFile(invalidPath);
    QVERIFY(invalidFile.open(QIODevice::WriteOnly | QIODevice::Text));
    invalidFile.write("Date;Label\n01/01/2025;Nothing\n"_ba);
    invalidFile.close();
    QVERIFY(!fileController->importFromCsv(QUrl::fromLocalFile(invalidPath)));
    QVERIFY(fileController->errorMessage().contains(u"Invalid CSV format"_s));

    const QString emptyDataPath = tempDir->filePath(u"no_rows.csv"_s);
    QFile emptyDataFile(emptyDataPath);
    QVERIFY(emptyDataFile.open(QIODevice::WriteOnly | QIODevice::Text));
    emptyDataFile.write("Date,Montant,Opération\n"_ba);
    emptyDataFile.close();
    QVERIFY(!fileController->importFromCsv(QUrl::fromLocalFile(emptyDataPath)));
    QVERIFY(fileController->errorMessage().isEmpty());
  }

  // Signals

  void testDataLoadedSignal() {
    QString filePath = tempDir->filePath(u"signal_test.comptine"_s);
    fileController->saveToYamlFile(filePath);

    QSignalSpy spy(fileController, &FileController::dataLoaded);
    fileController->loadFromYamlFile(filePath);

    QCOMPARE(spy.count(), 1);
  }

  void testYamlFileLoadedSignal() {
    QString filePath = tempDir->filePath(u"yaml_signal.comptine"_s);
    fileController->saveToYamlFile(filePath);

    QSignalSpy spy(fileController, &FileController::yamlFileLoaded);
    fileController->loadFromYamlFile(filePath);

    QCOMPARE(spy.count(), 1);
  }

  void testDataSavedSignal() {
    QString filePath = tempDir->filePath(u"save_signal.comptine"_s);

    QSignalSpy spy(fileController, &FileController::dataSaved);
    fileController->saveToYamlFile(filePath);

    QCOMPARE(spy.count(), 1);
  }

  void testFileExample() {
    QVERIFY(fileController->loadFromYamlUrl(QUrl(u"file::/tests/example.comptine"_s)));

    // Verify import
    QCOMPARE(budgetData->rowCount(), 2);
    auto account = budgetData->at(0);
    QCOMPARE(account->name(), u"Compte Courant"_s);
    QCOMPARE(account->operations().size(), 5);

    auto operation = account->operationAt(0);
    QCOMPARE(operation->date(), QDate(2025, 10, 8));
    QCOMPARE(operation->amount(), -45.0);
    QCOMPARE(operation->label(), u"Supermarche Carrefour"_s);
    QCOMPARE(operation->details(), u"Carte du 06/10/2025"_s);
    QCOMPARE(operation->allocations().count(), 2);

    auto allocation = operation->allocations().at(0);
    QCOMPARE(allocation->category()->name(), u"Alimentation"_s);
    QCOMPARE(allocation->amount(), -40.0);
    allocation = operation->allocations().at(1);
    QCOMPARE(allocation->category()->name(), u"Loisirs"_s);
    QCOMPARE(allocation->amount(), -5.0);

    operation = account->operationAt(1);
    QCOMPARE(operation->date(), QDate(2025, 10, 7));
    QCOMPARE(operation->amount(), -9.99);
    QCOMPARE(operation->label(), u"Abonnement Libération"_s);
    QCOMPARE(operation->allocations().count(), 1);
    auto alloc = operation->allocations().at(0);
    QCOMPARE(alloc->category()->name(), u"Loisirs"_s);
    QCOMPARE(alloc->amount(), -9.99);

    // Verify categories were created
    QCOMPARE(categoryController->rowCount(), 8);
    QVERIFY(categoryController->getCategoryByName(u"Alimentation"_s) != nullptr);
    QVERIFY(categoryController->getCategoryByName(u"Loisirs"_s) != nullptr);
  }

  void testFileOld() {
    QVERIFY(fileController->loadFromYamlUrl(QUrl(u"file::/tests/old.comptine"_s)));

    // Verify categories were created
    QCOMPARE(categoryController->rowCount(), 8);
    auto alimentation = categoryController->getCategoryByName(u"Alimentation"_s);
    QVERIFY(alimentation != nullptr);
    auto loisirs = categoryController->getCategoryByName(u"Loisirs"_s);
    QVERIFY(loisirs != nullptr);

    // Verify import
    QCOMPARE(budgetData->rowCount(), 2);
    auto account = budgetData->at(0);
    QCOMPARE(account->name(), u"Compte Courant"_s);
    QCOMPARE(account->operations().size(), 5);

    auto operation = account->operationAt(0);
    QCOMPARE(operation->date(), QDate(2025, 10, 8));
    QCOMPARE(operation->amount(), -45.0);
    QCOMPARE(operation->label(), u"Supermarche Carrefour"_s);
    QCOMPARE(operation->details(), u""_s);
    QCOMPARE(operation->allocations().count(), 2);

    auto allocation = operation->allocations().at(0);
    QCOMPARE(allocation->category(), alimentation);
    QCOMPARE(allocation->amount(), -40.0);
    allocation = operation->allocations().at(1);
    QCOMPARE(allocation->category(), loisirs);
    QCOMPARE(allocation->amount(), -5.0);

    operation = account->operationAt(1);
    QCOMPARE(operation->date(), QDate(2025, 10, 7));
    QCOMPARE(operation->amount(), -9.99);
    QCOMPARE(operation->label(), u"Abonnement Libération"_s);
    QCOMPARE(operation->allocations().count(), 1);

    allocation = operation->allocations().at(0);
    QCOMPARE(allocation->category(), loisirs);
    QCOMPARE(allocation->amount(), -9.99);
  }

  // CSV Import Integration

  void testFileImport1() {
    QVERIFY(fileController->importFromCsv(QUrl(u"file::/tests/import1.csv"_s), u"Bank Account"_s, true));

    // Verify import
    QCOMPARE(categoryController->rowCount(), 2);
    auto restaurant = categoryController->getCategoryByName(u"Restaurant"_s);
    QVERIFY(restaurant != nullptr);
    auto energie = categoryController->getCategoryByName(u"Energie eau, gaz, electricite, fioul"_s);
    QVERIFY(energie != nullptr);

    QCOMPARE(budgetData->rowCount(), 1);
    auto account = budgetData->at(0);
    QCOMPARE(account->name(), u"Bank Account"_s);
    QCOMPARE(account->operations().size(), 2);

    auto operation = account->operations().at(0);
    QCOMPARE(operation->date(), QDate(2025, 11, 27));
    QCOMPARE(operation->amount(), -35.0);
    QCOMPARE(operation->label(), u"LE PETIT BISTROT"_s);
    QCOMPARE(operation->details(), u"CB LE PETIT BISTRO FACT 251125"_s);
    QCOMPARE(operation->allocations().count(), 1);
    auto allocation = operation->allocations().at(0);
    QCOMPARE(allocation->category(), restaurant);
    QCOMPARE(allocation->amount(), -35.0);

    operation = account->operations().at(1);
    QCOMPARE(operation->date(), QDate(2025, 11, 18));
    QCOMPARE(operation->amount(), -85.0);
    QCOMPARE(operation->label(), u"EDF"_s);
    QCOMPARE(operation->allocations().count(), 1);
    allocation = operation->allocations().at(0);
    QCOMPARE(allocation->category(), energie);
    QCOMPARE(allocation->amount(), -85.0);
  }

  void testFileImport2() {
    QVERIFY(fileController->importFromCsv(QUrl(u"file::/tests/import2.csv"_s), u"Bank Account"_s, true));

    // Verify import
    QCOMPARE(categoryController->rowCount(), 0);

    QCOMPARE(budgetData->rowCount(), 1);
    auto account = budgetData->at(0);
    QCOMPARE(account->name(), u"Bank Account"_s);
    QCOMPARE(account->operations().size(), 1);

    auto operation = account->operations().at(0);
    QCOMPARE(operation->date(), QDate(2025, 6, 5));
    QCOMPARE(operation->amount(), -44.99);
    QCOMPARE(operation->label(), u"PRLV DE Free Telecom"_s);
    QCOMPARE(operation->allocations().count(), 0);
  }

  void testFileMoney() {
    QVERIFY(fileController->importFromCsv(QUrl(u"file::/tests/money.csv"_s), u"Bank Account"_s, true));

    // Verify import
    QCOMPARE(categoryController->rowCount(), 1);
    auto telephone = categoryController->getCategoryByName(u"Téléphone : Internet"_s);
    QVERIFY(telephone != nullptr);

    QCOMPARE(budgetData->rowCount(), 1);
    auto account = budgetData->at(0);
    QCOMPARE(account->name(), u"Bank Account"_s);
    QCOMPARE(account->operations().size(), 1);

    auto operation = account->operations().at(0);
    QCOMPARE(operation->date(), QDate(2025, 6, 5));
    QCOMPARE(operation->amount(), -44.99);
    QCOMPARE(operation->label(), u"PRLV DE Free Telecom"_s);
    QCOMPARE(operation->details(), u"PRLV Free Telecom Free HautDebit 1387145500"_s);
    // QCOMPARE(operation->category()->name(), "Téléphone : Internet"_L1);
    QCOMPARE(operation->allocations().count(), 1);
    auto allocation = operation->allocations().at(0);
    QCOMPARE(allocation->category(), telephone);
    QCOMPARE(allocation->amount(), -44.99);
  }

  void testFileMoney2() {
    QVERIFY(fileController->importFromCsv(QUrl(u"file::/tests/money2.csv"_s), u"Bank Account"_s, true));

    QCOMPARE(categoryController->rowCount(), 3);
    auto ameublement = categoryController->getCategoryByName(u"Factures : Ameublement"_s);
    QVERIFY(ameublement != nullptr);

    QCOMPARE(budgetData->rowCount(), 1);
    auto account = budgetData->at(0);
    QCOMPARE(account->name(), u"Bank Account"_s);
    QCOMPARE(account->operations().size(), 3);

    auto operation = account->operations().at(0);
    QCOMPARE(operation->date(), QDate(2025, 6, 24));
    QCOMPARE(operation->amount(), -24.5);
    QCOMPARE(operation->label(), u"VIREMENT SEPA PAR INTERNET"_s);
    QCOMPARE(operation->details(), u""_s);
    QCOMPARE(operation->allocations().count(), 1);
    auto allocation = operation->allocations().at(0);
    QCOMPARE(allocation->category(), ameublement);
    QCOMPARE(allocation->amount(), -24.5);
  }

  void testImportFromCsvWithCategories() {
    // Create a test CSV file
    QString csvPath = tempDir->filePath(u"import.csv"_s);
    QFile csvFile(csvPath);
    QVERIFY(csvFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&csvFile);
    out << u"Date,Montant,Opération,Catégorie\n"_s;
    out << u"15/01/2025,-50.00,Grocery Store,Food\n"_s;
    out << u"16/01/2025,-30.00,Bus Ticket,Transport\n"_s;
    csvFile.close();

    // Import with categories
    QUrl csvUrl = QUrl::fromLocalFile(csvPath);
    QVERIFY(fileController->importFromCsv(csvUrl, u"Bank Account"_s, true));

    // Verify import
    QCOMPARE(budgetData->rowCount(), 1);
    auto account = budgetData->at(0);
    QCOMPARE(account->name(), u"Bank Account"_s);
    QCOMPARE(account->operations().size(), 2);

    // Verify categories were created
    QCOMPARE(categoryController->rowCount(), 2);
    QVERIFY(categoryController->getCategoryByName(u"Food"_s) != nullptr);
    QVERIFY(categoryController->getCategoryByName(u"Transport"_s) != nullptr);
  }

  void testImportFromCsvWithoutCategories() {
    QString csvPath = tempDir->filePath(u"import_no_cat.csv"_s);
    QFile csvFile(csvPath);
    QVERIFY(csvFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&csvFile);
    out << u"Date,Montant,Opération,Catégorie\n"_s;
    out << u"20/02/2025,-100.00,Purchase,Shopping\n"_s;
    csvFile.close();

    // Import without categories
    QUrl csvUrl = QUrl::fromLocalFile(csvPath);
    QVERIFY(fileController->importFromCsv(csvUrl, u"Cash"_s, false));

    // Verify operation was imported but category was ignored
    auto account = budgetData->at(0);
    auto op = account->operations()[0];
    QCOMPARE(op->allocations().count(), 0);  // Empty category

    // No categories should be created
    QCOMPARE(categoryController->rowCount(), 0);
  }

  void testImportAppliesCategorizationRules() {
    // Create categorization rule
    auto groceries = categoryEditor->edit(u"Groceries"_s, 300.0);
    ruleController->addRule(new Rule(groceries, u"SUPERMARKET"_s));

    // Create CSV without category column
    QString csvPath = tempDir->filePath(u"import_rules.csv"_s);
    QFile csvFile(csvPath);
    QVERIFY(csvFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&csvFile);
    out << u"Date,Montant,Opération\n"_s;
    out << u"10/03/2025,-45.00,SUPERMARKET PURCHASE\n"_s;
    csvFile.close();

    // Import
    QUrl csvUrl = QUrl::fromLocalFile(csvPath);
    fileController->importFromCsv(csvUrl, u"Account"_s);

    // Rule should have been applied
    auto op = budgetData->at(0)->operations()[0];
    QVERIFY(op);
    QCOMPARE(op->label(), u"SUPERMARKET PURCHASE"_s);
    QCOMPARE(op->allocations().size(), 1);
    auto alloc = op->allocations().at(0);
    QCOMPARE(alloc->category(), groceries);
    QCOMPARE(alloc->amount(), -45.0);
  }

private:
  QTemporaryDir* tempDir;
  QUndoStack* undoStack;
  AppSettings* appSettings;
  BudgetData* budgetData;
  CategoryController* categoryController;
  CategoryEditor* categoryEditor;
  RuleController* ruleController;
  FileController* fileController;
};

QTEST_GUILESS_MAIN(FileControllerTest)
#include "FileControllerTest.moc"
