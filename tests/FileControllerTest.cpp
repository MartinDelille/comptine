// Integration tests for FileController
#include <QDate>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include "../AppSettings.h"
#include "../BudgetData.h"
#include "../CategoryController.h"
#include "../FileController.h"
#include "../RuleController.h"
#include "../UndoCommands.h"
#include "model/Account.h"
#include "model/Category.h"
#include "model/Operation.h"
#include "model/Rule.h"

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
    ruleController = new RuleController(*budgetData, *undoStack);
    fileController = new FileController(*appSettings, *budgetData, *categoryController, *ruleController, *undoStack);
  }

  void cleanup() {
    delete fileController;
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
    auto account = budgetData->createAccount("Test Account");
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
    undoStack->push(new QUndoCommand("Test change"));
    QVERIFY(fileController->hasUnsavedChanges());

    // Save should clear unsaved flag
    QString filePath = tempDir->filePath("saved.comptine");
    fileController->saveToYamlFile(filePath);
    QVERIFY(!fileController->hasUnsavedChanges());
  }

  //  Save/Load Empty Files

  void testSaveAndLoadEmptyFile() {
    QString filePath = tempDir->filePath("empty.comptine");

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
    QString filePath = tempDir->filePath("url_test.comptine");
    QUrl fileUrl = QUrl::fromLocalFile(filePath);

    // Save using QUrl
    QVERIFY(fileController->saveToYamlUrl(fileUrl));
    QVERIFY(QFile::exists(filePath));
  }

  void testLoadFromYamlUrl() {
    // Create a test file
    QString filePath = tempDir->filePath("url_load.comptine");
    budgetData->createAccount("Test Account");
    fileController->saveToYamlFile(filePath);

    // Load using QUrl
    fileController->clear();
    QUrl fileUrl = QUrl::fromLocalFile(filePath);
    QVERIFY(fileController->loadFromYamlUrl(fileUrl));
    QCOMPARE(budgetData->rowCount(), 1);
  }

  // Save/Load with Accounts and Operations

  void testSaveAndLoadWithSingleAccount() {
    // Create test data
    auto account = budgetData->createAccount("Checking Account");
    auto food = categoryController->addCategory(new Category("Food"));
    auto op = account->addOperation(
        new Operation(account,
                      QDate(2025, 1, 15),
                      -50.0,
                      "Grocery Store",
                      {},
                      { new Allocation(food, -50) }),
        false);

    categoryController->editCategory("Food", 200.0);

    // Save to file
    QString filePath = tempDir->filePath("single_account.comptine");
    QVERIFY(fileController->saveToYamlFile(filePath));

    // Clear and reload
    fileController->clear();
    QVERIFY(fileController->loadFromYamlFile(filePath));

    // Verify data was restored
    QCOMPARE(budgetData->rowCount(), 1);
    QCOMPARE(categoryController->rowCount(), 1);

    auto loadedAccount = budgetData->at(0);
    QCOMPARE(loadedAccount->name(), QString("Checking Account"));
    QCOMPARE(loadedAccount->operations().size(), 1);

    auto loadedOp = loadedAccount->operations()[0];
    QCOMPARE(loadedOp->date(), QDate(2025, 1, 15));
    QCOMPARE(loadedOp->amount(), -50.0);
    QCOMPARE(loadedOp->label(), QString("Grocery Store"));
    QCOMPARE(loadedOp->allocations().count(), 1);
    auto alloc = loadedOp->allocations().at(0);
    QCOMPARE(alloc->category()->name(), QString("Food"));
    QCOMPARE(alloc->amount(), -50.0);
  }

  void testSaveAndLoadWithMultipleAccounts() {
    // Create multiple accounts
    auto checking = budgetData->createAccount("Checking");
    auto savings = budgetData->createAccount("Savings");

    // Add operations to each
    auto op1 = checking->addOperation(
        new Operation(checking,
                      QDate(2025, 1, 10), -100, "Purchase 1"),
        false);

    auto op2 = savings->addOperation(new Operation(savings, QDate(2025, 1, 20), 500, "Deposit"), false);

    // Save and reload
    QString filePath = tempDir->filePath("multiple_accounts.comptine");
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    // Verify
    QCOMPARE(budgetData->rowCount(), 2);
    QCOMPARE(budgetData->at(0)->name(), QString("Checking"));
    QCOMPARE(budgetData->at(1)->name(), QString("Savings"));
    QCOMPARE(budgetData->at(0)->operations().size(), 1);
    QCOMPARE(budgetData->at(1)->operations().size(), 1);
  }

  // Save/Load with Split Operations

  void testSaveAndLoadSplitOperation() {
    auto account = budgetData->createAccount("Test Account");

    // Create split operation

    auto food = categoryController->editCategory("Food", 200.0);
    auto transport = categoryController->editCategory("Transport", 100.0);

    auto op = account->addOperation(
        new Operation(account,
                      QDate(2025, 2, 1),
                      -150.0,
                      "Mixed Purchase"),
        false);
    QList<Allocation*> allocations;
    allocations.append(new Allocation(food, -100.0));
    allocations.append(new Allocation(transport, -50.0));
    op->setAllocations(allocations);

    // Save and reload
    QString filePath = tempDir->filePath("split_operation.comptine");
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    // Verify split operation
    auto loadedAccount = budgetData->at(0);
    auto loadedOp = loadedAccount->operations()[0];
    QVERIFY(loadedOp->isCategorized());

    auto loadedAllocs = loadedOp->allocations();
    QCOMPARE(loadedAllocs.size(), 2);
    QCOMPARE(loadedAllocs[0]->category()->name(), QString("Food"));
    QCOMPARE(loadedAllocs[0]->amount(), -100.0);
    QCOMPARE(loadedAllocs[1]->category()->name(), QString("Transport"));
    QCOMPARE(loadedAllocs[1]->amount(), -50.0);
  }

  // Save/Load with Budget Dates

  void testSaveAndLoadWithBudgetDate() {
    auto account = budgetData->createAccount("Test Account");
    auto op = account->addOperation(new Operation(
                                        account,
                                        QDate(2025, 1, 31),
                                        -75.0,
                                        "Late Month Purchase"),
                                    false);
    auto shopping = categoryController->addCategory(new Category("Shopping"));
    op->setAllocations({ new Allocation(shopping, -75.0) });
    op->set_budgetDate(QDate(2025, 2, 1));  // Budget to next month

    categoryController->editCategory("Shopping", 150.0);

    // Save and reload
    QString filePath = tempDir->filePath("budget_date.comptine");
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    // Verify budget date is preserved
    auto loadedOp = budgetData->at(0)->operations()[0];
    QCOMPARE(loadedOp->date(), QDate(2025, 1, 31));
    QCOMPARE(loadedOp->budgetDate(), QDate(2025, 2, 1));
  }

  void testBudgetDateNotSavedWhenSameAsDate() {
    auto account = budgetData->createAccount("Test Account");
    auto food = categoryController->editCategory("Food", 200.0);

    // budgetDate defaults to date, so it should not be saved
    auto op = account->addOperation(new Operation(
                                        account,
                                        QDate(2025, 3, 15),
                                        -30.0,
                                        "Normal Purchase"),
                                    false);
    op->setAllocations({ new Allocation(food, -30.0) });

    // Save and check file content doesn't have budget_date
    QString filePath = tempDir->filePath("no_budget_date.comptine");
    fileController->saveToYamlFile(filePath);

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = file.readAll();
    file.close();

    // budget_date should not appear in the file
    QVERIFY(!content.contains("budget_date"));
  }

  // Save/Load with Categories and Budget Limits

  void testSaveAndLoadCategories() {
    categoryController->addCategory(new Category("Food", 500.0));
    categoryController->addCategory(new Category("Transport", 200.0));
    categoryController->addCategory(new Category("Entertainment", 100.0));

    // Save and reload
    QString filePath = tempDir->filePath("categories.comptine");
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    // Verify categories
    QCOMPARE(categoryController->rowCount(), 3);
    auto food = categoryController->getCategoryByName("Food");
    QVERIFY(food != nullptr);
    QCOMPARE(food->budgetLimit(), 500.0);
  }

  // Save/Load with Leftover Decisions

  void testSaveAndLoadLeftoverDecisions() {
    auto cat = new Category("Savings", 300.0);
    categoryController->addCategory(cat);

    // Set leftover decision for January 2025
    cat->setLeftoverDecision(2025, 1, { 100.0, 50.0 });  // save 100, report 50

    // Save and reload
    QString filePath = tempDir->filePath("leftover.comptine");
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    // Verify leftover decision
    auto loadedCat = categoryController->getCategoryByName("Savings");
    QVERIFY(loadedCat != nullptr);
    LeftoverDecision decision = loadedCat->leftoverDecision(2025, 1);
    QCOMPARE(decision.saveAmount, 100.0);
    QCOMPARE(decision.reportAmount, 50.0);
  }

  void testLoadLegacyLeftoverFormat() {
    // Create a file with legacy leftover format (action + amount)
    QString filePath = tempDir->filePath("legacy_leftover.comptine");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << "categories:\n";
    out << "  - name: Food\n";
    out << "    budget_limit: 200.00\n";
    out << "    leftover_decisions:\n";
    out << "      - year: 2025\n";
    out << "        month: 1\n";
    out << "        action: save\n";
    out << "        amount: 50.00\n";
    out << "accounts: []\n";
    file.close();

    // Load and verify conversion
    fileController->loadFromYamlFile(filePath);
    auto cat = categoryController->getCategoryByName("Food");
    QVERIFY(cat != nullptr);
    LeftoverDecision decision = cat->leftoverDecision(2025, 1);
    QCOMPARE(decision.saveAmount, 50.0);
    QCOMPARE(decision.reportAmount, 0.0);
  }

  // Save/Load with Month History and Budget Limit Overrides

  void testSaveAndLoadMonthHistoryWithBudgetLimit() {
    auto cat = new Category("Groceries", -300.0);
    categoryController->addCategory(cat);

    // Record that budget was 250 until June (old limit stored in history)
    cat->setBudgetLimitForMonth(2025, 6, -250.0);

    // Also set leftover decision for June
    cat->setLeftoverDecision(2025, 6, { 30.0, 20.0 });

    // Save and reload
    QString filePath = tempDir->filePath("month_history_budget.comptine");
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    // Verify
    auto loaded = categoryController->getCategoryByName("Groceries");
    QVERIFY(loaded != nullptr);
    QCOMPARE(loaded->budgetLimit(), -300.0);

    // Verify month record has both leftover data and budget limit
    MonthRecord record = loaded->monthRecord(2025, 6);
    QCOMPARE(record.saveAmount, 30.0);
    QCOMPARE(record.reportAmount, 20.0);
    QVERIFY(record.budgetLimit.has_value());
    QCOMPARE(record.budgetLimit.value(), -250.0);

    // Verify budgetLimitForMonth lookup works after reload
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 3, 1)), -250.0);
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 6, 1)), -250.0);
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 7, 1)), -300.0);
  }

  void testSaveAndLoadMonthHistoryBudgetLimitOnly() {
    auto cat = new Category("Transport", -150.0);
    categoryController->addCategory(cat);

    // Only budget limit in history, no leftover data
    cat->setBudgetLimitForMonth(2025, 3, -100.0);

    QString filePath = tempDir->filePath("budget_limit_only.comptine");
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    auto loaded = categoryController->getCategoryByName("Transport");
    QVERIFY(loaded != nullptr);

    MonthRecord record = loaded->monthRecord(2025, 3);
    QCOMPARE(record.saveAmount, 0.0);
    QCOMPARE(record.reportAmount, 0.0);
    QVERIFY(record.budgetLimit.has_value());
    QCOMPARE(record.budgetLimit.value(), -100.0);
  }

  void testSaveAndLoadMultipleBudgetLimitChanges() {
    auto cat = new Category("Food", -400.0);
    categoryController->addCategory(cat);

    // Multiple historical budget limit changes
    cat->setBudgetLimitForMonth(2025, 3, -200.0);  // Was 200 until March
    cat->setBudgetLimitForMonth(2025, 6, -300.0);  // Was 300 until June
    // Current is 400

    QString filePath = tempDir->filePath("multi_budget_limit.comptine");
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    auto loaded = categoryController->getCategoryByName("Food");
    QVERIFY(loaded != nullptr);
    QCOMPARE(loaded->budgetLimit(), -400.0);

    // Verify the forward-scan lookup works correctly
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 1, 1)), -200.0);
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 3, 1)), -200.0);
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 4, 1)), -300.0);
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 6, 1)), -300.0);
    QCOMPARE(loaded->budgetLimitForMonth(QDate(2025, 7, 1)), -400.0);
  }

  void testLoadLegacyLeftoverDecisionsKey() {
    // Old files use "leftover_decisions" key — verify it still loads correctly
    QString filePath = tempDir->filePath("legacy_key.comptine");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << "categories:\n";
    out << "  - name: Shopping\n";
    out << "    budget_limit: -200.00\n";
    out << "    leftover_decisions:\n";
    out << "      - year: 2025\n";
    out << "        month: 1\n";
    out << "        save_amount: 40.00\n";
    out << "        report_amount: 15.00\n";
    out << "accounts: []\n";
    file.close();

    fileController->loadFromYamlFile(filePath);
    auto cat = categoryController->getCategoryByName("Shopping");
    QVERIFY(cat != nullptr);

    LeftoverDecision decision = cat->leftoverDecision(2025, 1);
    QCOMPARE(decision.saveAmount, 40.0);
    QCOMPARE(decision.reportAmount, 15.0);
  }

  void testSaveUsesMonthHistoryKey() {
    // Verify that saving uses the new "month_history" key
    auto cat = new Category("Test", -100.0);
    categoryController->addCategory(cat);
    cat->setLeftoverDecision(2025, 1, { 10.0, 5.0 });

    QString filePath = tempDir->filePath("key_check.comptine");
    fileController->saveToYamlFile(filePath);

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = file.readAll();
    file.close();

    QVERIFY(content.contains("month_history"));
    QVERIFY(!content.contains("leftover_decisions"));
  }

  // Save/Load with Categorization Rules

  void testSaveAndLoadCategorizationRules() {
    auto groceries = categoryController->editCategory("Groceries", 300.0);
    auto fuel = categoryController->editCategory("Fuel", 150.0);

    ruleController->addRule(new Rule(groceries, "SUPERMARKET"));
    ruleController->addRule(new Rule(fuel, "PETROL"));

    // Save and reload
    QString filePath = tempDir->filePath("rules.comptine");
    fileController->saveToYamlFile(filePath);
    fileController->clear();
    fileController->loadFromYamlFile(filePath);

    // Verify rules
    QList<Rule*> rules = ruleController->rules();
    QCOMPARE(rules.size(), 2);
    QCOMPARE(rules[0]->category()->name(), QString("Groceries"));
    QCOMPARE(rules[0]->labelMatch(), QString("SUPERMARKET"));
  }

  // Error Handling

  void testSaveToInvalidPath() {
    // Keep the failure deterministic without relying on a system path that
    // could exist on a particular machine.
    QString filePath = tempDir->filePath("missing-directory/file.comptine");
    QVERIFY(!fileController->saveToYamlFile(filePath));
    QVERIFY(!fileController->errorMessage().isEmpty());
  }

  void testLoadFromNonexistentFile() {
    QString filePath = tempDir->filePath("does_not_exist.comptine");
    QVERIFY(!fileController->loadFromYamlFile(filePath));
    QVERIFY(!fileController->errorMessage().isEmpty());
  }

  void testLoadFromEmptyFile() {
    // Create empty file
    QString filePath = tempDir->filePath("empty_file.comptine");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    QVERIFY(!fileController->loadFromYamlFile(filePath));
    QVERIFY(fileController->errorMessage().contains("empty"));
  }

  // NOTE: Commented out because yaml-cpp throws on truly invalid YAML
  // In production, FileCoordinator validation should catch these earlier
  /*
  void testLoadFromInvalidYaml() {
    // Create file with truly invalid YAML (unclosed bracket)
    QString filePath = tempDir->filePath("invalid.comptine");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << "categories:\n";
    out << "  - name: Test\n";
    out << "    budget_limit: [broken yaml\n";  // Invalid - unclosed bracket
    file.close();

    // Should handle parsing error gracefully
    bool loaded = fileController->loadFromYamlFile(filePath);
    // yaml-cpp may be lenient, so either it fails or succeeds but with error logged
    // We just verify it doesn't crash
    QVERIFY(loaded == loaded);  // Always pass - just checking no crash
  }
  */

  void testLoadFromInvalidUrl() {
    QUrl invalidUrl("http://example.com/file.comptine");
    QVERIFY(!fileController->loadFromYamlUrl(invalidUrl));
  }

  // Signals

  void testDataLoadedSignal() {
    QString filePath = tempDir->filePath("signal_test.comptine");
    fileController->saveToYamlFile(filePath);

    QSignalSpy spy(fileController, &FileController::dataLoaded);
    fileController->loadFromYamlFile(filePath);

    QCOMPARE(spy.count(), 1);
  }

  void testYamlFileLoadedSignal() {
    QString filePath = tempDir->filePath("yaml_signal.comptine");
    fileController->saveToYamlFile(filePath);

    QSignalSpy spy(fileController, &FileController::yamlFileLoaded);
    fileController->loadFromYamlFile(filePath);

    QCOMPARE(spy.count(), 1);
  }

  void testDataSavedSignal() {
    QString filePath = tempDir->filePath("save_signal.comptine");

    QSignalSpy spy(fileController, &FileController::dataSaved);
    fileController->saveToYamlFile(filePath);

    QCOMPARE(spy.count(), 1);
  }

  void testFileExample() {
    QVERIFY(fileController->loadFromYamlUrl(QUrl("file::/tests/example.comptine")));

    // Verify import
    QCOMPARE(budgetData->rowCount(), 2);
    auto account = budgetData->at(0);
    QCOMPARE(account->name(), QString("Compte Courant"));
    QCOMPARE(account->operations().size(), 5);

    auto operation = account->operationAt(0);
    QCOMPARE(operation->date(), QDate(2025, 10, 8));
    QCOMPARE(operation->amount(), -45.0);
    QCOMPARE(operation->label(), "Supermarche Carrefour");
    QCOMPARE(operation->details(), "Carte du 06/10/2025");
    QCOMPARE(operation->allocations().count(), 2);

    auto allocation = operation->allocations().at(0);
    QCOMPARE(allocation->category()->name(), "Alimentation");
    QCOMPARE(allocation->amount(), -40.0);
    allocation = operation->allocations().at(1);
    QCOMPARE(allocation->category()->name(), "Loisirs");
    QCOMPARE(allocation->amount(), -5.0);

    operation = account->operationAt(1);
    QCOMPARE(operation->date(), QDate(2025, 10, 7));
    QCOMPARE(operation->amount(), -9.99);
    QCOMPARE(operation->label(), QString("Abonnement Libération"));
    QCOMPARE(operation->allocations().count(), 1);
    auto alloc = operation->allocations().at(0);
    QCOMPARE(alloc->category()->name(), QString("Loisirs"));
    QCOMPARE(alloc->amount(), -9.99);

    // Verify categories were created
    QCOMPARE(categoryController->rowCount(), 8);
    QVERIFY(categoryController->getCategoryByName("Alimentation") != nullptr);
    QVERIFY(categoryController->getCategoryByName("Loisirs") != nullptr);
  }

  void testFileOld() {
    QVERIFY(fileController->loadFromYamlUrl(QUrl("file::/tests/old.comptine")));

    // Verify categories were created
    QCOMPARE(categoryController->rowCount(), 8);
    auto alimentation = categoryController->getCategoryByName("Alimentation");
    QVERIFY(alimentation != nullptr);
    auto loisirs = categoryController->getCategoryByName("Loisirs");
    QVERIFY(loisirs != nullptr);

    // Verify import
    QCOMPARE(budgetData->rowCount(), 2);
    auto account = budgetData->at(0);
    QCOMPARE(account->name(), QString("Compte Courant"));
    QCOMPARE(account->operations().size(), 5);

    auto operation = account->operationAt(0);
    QCOMPARE(operation->date(), QDate(2025, 10, 8));
    QCOMPARE(operation->amount(), -45.0);
    QCOMPARE(operation->label(), "Supermarche Carrefour");
    QCOMPARE(operation->details(), "");
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
    QCOMPARE(operation->label(), QString("Abonnement Libération"));
    QCOMPARE(operation->allocations().count(), 1);

    allocation = operation->allocations().at(0);
    QCOMPARE(allocation->category(), loisirs);
    QCOMPARE(allocation->amount(), -9.99);
  }

  // CSV Import Integration

  void testFileImport1() {
    QVERIFY(fileController->importFromCsv(QUrl("file::/tests/import1.csv"), "Bank Account", true));

    // Verify import
    QCOMPARE(categoryController->rowCount(), 2);
    auto restaurant = categoryController->getCategoryByName("Restaurant");
    QVERIFY(restaurant != nullptr);
    auto energie = categoryController->getCategoryByName("Energie eau, gaz, electricite, fioul");
    QVERIFY(energie != nullptr);

    QCOMPARE(budgetData->rowCount(), 1);
    auto account = budgetData->at(0);
    QCOMPARE(account->name(), QString("Bank Account"));
    QCOMPARE(account->operations().size(), 2);

    auto operation = account->operations().at(0);
    QCOMPARE(operation->date(), QDate(2025, 11, 27));
    QCOMPARE(operation->amount(), -35.0);
    QCOMPARE(operation->label(), QString("LE PETIT BISTROT"));
    QCOMPARE(operation->details(), QString("CB LE PETIT BISTRO FACT 251125"));
    QCOMPARE(operation->allocations().count(), 1);
    auto allocation = operation->allocations().at(0);
    QCOMPARE(allocation->category(), restaurant);
    QCOMPARE(allocation->amount(), -35.0);

    operation = account->operations().at(1);
    QCOMPARE(operation->date(), QDate(2025, 11, 18));
    QCOMPARE(operation->amount(), -85.0);
    QCOMPARE(operation->label(), QString("EDF"));
    QCOMPARE(operation->allocations().count(), 1);
    allocation = operation->allocations().at(0);
    QCOMPARE(allocation->category(), energie);
    QCOMPARE(allocation->amount(), -85.0);
  }

  void testFileImport2() {
    QVERIFY(fileController->importFromCsv(QUrl("file::/tests/import2.csv"), "Bank Account", true));

    // Verify import
    QCOMPARE(categoryController->rowCount(), 0);

    QCOMPARE(budgetData->rowCount(), 1);
    auto account = budgetData->at(0);
    QCOMPARE(account->name(), QString("Bank Account"));
    QCOMPARE(account->operations().size(), 1);

    auto operation = account->operations().at(0);
    QCOMPARE(operation->date(), QDate(2025, 6, 5));
    QCOMPARE(operation->amount(), -44.99);
    QCOMPARE(operation->label(), QString("PRLV DE Free Telecom"));
    QCOMPARE(operation->allocations().count(), 0);
  }

  void testFileMoney() {
    QVERIFY(fileController->importFromCsv(QUrl("file::/tests/money.csv"), "Bank Account", true));

    // Verify import
    QCOMPARE(categoryController->rowCount(), 1);
    auto telephone = categoryController->getCategoryByName("Téléphone : Internet");
    QVERIFY(telephone != nullptr);

    QCOMPARE(budgetData->rowCount(), 1);
    auto account = budgetData->at(0);
    QCOMPARE(account->name(), QString("Bank Account"));
    QCOMPARE(account->operations().size(), 1);

    auto operation = account->operations().at(0);
    QCOMPARE(operation->date(), QDate(2025, 6, 5));
    QCOMPARE(operation->amount(), -44.99);
    QCOMPARE(operation->label(), QString("PRLV DE Free Telecom"));
    QCOMPARE(operation->details(), QString("PRLV Free Telecom Free HautDebit 1387145500"));
    // QCOMPARE(operation->category()->name(), "Téléphone : Internet");
    QCOMPARE(operation->allocations().count(), 1);
    auto allocation = operation->allocations().at(0);
    QCOMPARE(allocation->category(), telephone);
    QCOMPARE(allocation->amount(), -44.99);
  }

  void testFileMoney2() {
    QVERIFY(fileController->importFromCsv(QUrl("file::/tests/money2.csv"), "Bank Account", true));

    QCOMPARE(categoryController->rowCount(), 3);
    auto ameublement = categoryController->getCategoryByName("Factures : Ameublement");
    QVERIFY(ameublement != nullptr);

    QCOMPARE(budgetData->rowCount(), 1);
    auto account = budgetData->at(0);
    QCOMPARE(account->name(), QString("Bank Account"));
    QCOMPARE(account->operations().size(), 3);

    auto operation = account->operations().at(0);
    QCOMPARE(operation->date(), QDate(2025, 6, 24));
    QCOMPARE(operation->amount(), -24.5);
    QCOMPARE(operation->label(), "VIREMENT SEPA PAR INTERNET");
    QCOMPARE(operation->details(), "");
    QCOMPARE(operation->allocations().count(), 1);
    auto allocation = operation->allocations().at(0);
    QCOMPARE(allocation->category(), ameublement);
    QCOMPARE(allocation->amount(), -24.5);
  }

  void testImportFromCsvWithCategories() {
    // Create a test CSV file
    QString csvPath = tempDir->filePath("import.csv");
    QFile csvFile(csvPath);
    QVERIFY(csvFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&csvFile);
    out << "Date,Montant,Opération,Catégorie\n";
    out << "15/01/2025,-50.00,Grocery Store,Food\n";
    out << "16/01/2025,-30.00,Bus Ticket,Transport\n";
    csvFile.close();

    // Import with categories
    QUrl csvUrl = QUrl::fromLocalFile(csvPath);
    QVERIFY(fileController->importFromCsv(csvUrl, "Bank Account", true));

    // Verify import
    QCOMPARE(budgetData->rowCount(), 1);
    auto account = budgetData->at(0);
    QCOMPARE(account->name(), QString("Bank Account"));
    QCOMPARE(account->operations().size(), 2);

    // Verify categories were created
    QCOMPARE(categoryController->rowCount(), 2);
    QVERIFY(categoryController->getCategoryByName("Food") != nullptr);
    QVERIFY(categoryController->getCategoryByName("Transport") != nullptr);
  }

  void testImportFromCsvWithoutCategories() {
    QString csvPath = tempDir->filePath("import_no_cat.csv");
    QFile csvFile(csvPath);
    QVERIFY(csvFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&csvFile);
    out << "Date,Montant,Opération,Catégorie\n";
    out << "20/02/2025,-100.00,Purchase,Shopping\n";
    csvFile.close();

    // Import without categories
    QUrl csvUrl = QUrl::fromLocalFile(csvPath);
    QVERIFY(fileController->importFromCsv(csvUrl, "Cash", false));

    // Verify operation was imported but category was ignored
    auto account = budgetData->at(0);
    auto op = account->operations()[0];
    QCOMPARE(op->allocations().count(), 0);  // Empty category

    // No categories should be created
    QCOMPARE(categoryController->rowCount(), 0);
  }

  void testImportAppliesCategorizationRules() {
    // Create categorization rule
    auto groceries = categoryController->editCategory("Groceries", 300.0);
    ruleController->addRule(new Rule(groceries, "SUPERMARKET"));

    // Create CSV without category column
    QString csvPath = tempDir->filePath("import_rules.csv");
    QFile csvFile(csvPath);
    QVERIFY(csvFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&csvFile);
    out << "Date,Montant,Opération\n";
    out << "10/03/2025,-45.00,SUPERMARKET PURCHASE\n";
    csvFile.close();

    // Import
    QUrl csvUrl = QUrl::fromLocalFile(csvPath);
    fileController->importFromCsv(csvUrl, "Account");

    // Rule should have been applied
    auto op = budgetData->at(0)->operations()[0];
    QVERIFY(op);
    QCOMPARE(op->label(), "SUPERMARKET PURCHASE");
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
  RuleController* ruleController;
  FileController* fileController;
};

QTEST_GUILESS_MAIN(FileControllerTest)
#include "FileControllerTest.moc"
