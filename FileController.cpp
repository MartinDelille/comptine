// Must be included before Qt headers to avoid 'emit' keyword conflict
#define RYML_NO_DEFAULT_CALLBACKS
#include <c4/format.hpp>
#include <ryml.hpp>
#include <ryml_std.hpp>

#include <QDate>
#include <QDebug>
#include <QFile>
#include <QTextStream>

#include "Account.h"
#include "AppSettings.h"
#include "BudgetData.h"
#include "CategorizationRule.h"
#include "Category.h"
#include "CategoryController.h"
#include "CsvParser.h"
#include "FileController.h"
#include "FileCoordinator.h"
#include "NavigationController.h"
#include "Operation.h"
#include "RuleController.h"
#include "UndoCommands.h"

using namespace CsvParser;

FileController::FileController(AppSettings& appSettings,
                               BudgetData& budgetData,
                               CategoryController& categoryController,
                               NavigationController& navController,
                               RuleController& ruleController,
                               QUndoStack& undoStack) :
    _appSettings(appSettings),
    _budgetData(budgetData),
    _categoryController(categoryController),
    _navController(navController),
    _ruleController(ruleController),
    _undoStack(undoStack) {
}

bool FileController::hasUnsavedChanges() const {
  return !_undoStack.isClean();
}

// Helper to convert QString to std::string for ryml
static std::string toStdString(const QString& s) {
  return s.toStdString();
}

bool FileController::saveToYamlUrl(const QUrl& fileUrl) {
  const QString filePath = fileUrl.toLocalFile();
  if (filePath.isEmpty()) {
    qWarning() << "Invalid or unsupported QUrl passed to saveToYaml:" << fileUrl;
    set_errorMessage(tr("Invalid or unsupported file path."));
    return false;
  }
  saveToYamlFile(filePath);
  return true;
}

bool FileController::saveToYamlFile(const QString& filePath) {
  // Clear any previous error
  set_errorMessage({});

  ryml::Tree tree;
  ryml::NodeRef root = tree.rootref();
  root |= ryml::MAP;

  // Get navigation state from NavigationController
  int currentTabIndex = _navController.currentTabIndex();
  int budgetYear = _navController.budgetYear();
  int budgetMonth = _navController.budgetMonth();
  int currentAccountIndex = _navController.currentAccountIndex();
  int currentCategoryIndex = _navController.currentCategoryIndex();

  // Write state section
  ryml::NodeRef state = root["state"];
  state |= ryml::MAP;
  state["currentTab"] << currentTabIndex;
  state["budgetYear"] << budgetYear;
  state["budgetMonth"] << budgetMonth;

  // Write categories
  ryml::NodeRef categories = root["categories"];
  categories |= ryml::SEQ;
  QList<Category*> cats = _categoryController.categories();
  for (int i = 0; i < cats.size(); i++) {
    const Category* category = cats[i];
    ryml::NodeRef cat = categories.append_child();
    cat |= ryml::MAP;
    cat["name"] << toStdString(category->name());
    cat["budget_limit"] << toStdString(QString::number(category->budgetLimit(), 'f', 2));
    if (i == currentCategoryIndex) {
      cat["current"] << "true";
    }

    // Write leftover decisions
    QMap<YearMonth, LeftoverDecision> decisions = category->allLeftoverDecisions();
    if (!decisions.isEmpty()) {
      ryml::NodeRef leftoverNode = cat["leftover_decisions"];
      leftoverNode |= ryml::SEQ;
      for (auto it = decisions.constBegin(); it != decisions.constEnd(); ++it) {
        const YearMonth& ym = it.key();
        const LeftoverDecision& decision = it.value();
        if (!decision.isEmpty()) {
          ryml::NodeRef decisionNode = leftoverNode.append_child();
          decisionNode |= ryml::MAP;
          decisionNode["year"] << ym.year;
          decisionNode["month"] << ym.month;
          if (decision.saveAmount != 0.0) {
            decisionNode["save_amount"] << toStdString(QString::number(decision.saveAmount, 'f', 2));
          }
          if (decision.reportAmount != 0.0) {
            decisionNode["report_amount"] << toStdString(QString::number(decision.reportAmount, 'f', 2));
          }
        }
      }
    }
  }

  // Write accounts
  ryml::NodeRef accounts = root["accounts"];
  accounts |= ryml::SEQ;
  QList<Account*> accs = _budgetData.accounts();
  for (int accIdx = 0; accIdx < accs.size(); accIdx++) {
    const Account* account = accs[accIdx];
    ryml::NodeRef acc = accounts.append_child();
    acc |= ryml::MAP;
    acc["name"] << toStdString(account->name());
    if (accIdx == currentAccountIndex) {
      acc["current"] << "true";
    }

    ryml::NodeRef operations = acc["operations"];
    operations |= ryml::SEQ;
    const auto& ops = account->operations();
    for (int opIdx = 0; opIdx < ops.size(); opIdx++) {
      const Operation* op = ops[opIdx];
      ryml::NodeRef opNode = operations.append_child();
      opNode |= ryml::MAP;
      opNode["date"] << toStdString(op->date().toString("yyyy-MM-dd"));
      opNode["amount"] << toStdString(QString::number(op->amount(), 'f', 2));
      opNode["description"] << toStdString(op->description());

      // Handle split operations vs single category
      if (op->isSplit()) {
        ryml::NodeRef allocsNode = opNode["allocations"];
        allocsNode |= ryml::SEQ;
        for (const auto& alloc : op->allocationsList()) {
          ryml::NodeRef allocNode = allocsNode.append_child();
          allocNode |= ryml::MAP;
          if (alloc.category) {
            allocNode["category"] << toStdString(alloc.category->name());
          }
          allocNode["amount"] << toStdString(QString::number(alloc.amount, 'f', 2));
        }
      } else if (op->category()) {
        opNode["category"] << toStdString(op->category()->name());
      }

      // Only save budget_date if explicitly set (different from operation date)
      if (op->budgetDate() != op->date()) {
        opNode["budget_date"] << toStdString(op->budgetDate().toString("yyyy-MM-dd"));
      }
      // Mark current operation for this account
      if (op == account->currentOperation()) {
        opNode["current"] << "true";
      }
    }
  }

  // Write categorization rules
  QList<CategorizationRule*> rulesList = _ruleController.rules();
  if (!rulesList.isEmpty()) {
    ryml::NodeRef rules = root["rules"];
    rules |= ryml::SEQ;
    for (const CategorizationRule* rule : rulesList) {
      ryml::NodeRef ruleNode = rules.append_child();
      ruleNode |= ryml::MAP;
      if (rule->category()) {
        ruleNode["category"] << toStdString(rule->category()->name());
        ruleNode["description_prefix"] << toStdString(rule->descriptionPrefix());
      }
    }
  }

  // Emit YAML to string
  std::string yaml = ryml::emitrs_yaml<std::string>(tree);

  // Write to file
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    qWarning() << "Failed to open file for writing:" << filePath;
    set_errorMessage(tr("Could not save file: %1").arg(file.errorString()));
    return false;
  }

  QTextStream out(&file);
  out.setEncoding(QStringConverter::Utf8);
  out << QString::fromStdString(yaml);
  file.close();

  qDebug() << "Budget data saved to:" << filePath;
  _undoStack.setClean();
  emit dataSaved();
  set_currentFilePath(filePath);
  _appSettings.addRecentFile(filePath);

  return true;
}

bool FileController::loadFromYamlUrl(const QUrl& fileUrl) {
  QString filePath = fileUrl.toLocalFile();
  if (filePath.isEmpty()) {
    qWarning() << "Invalid or unsupported QUrl passed to loadFromYamlUrl:" << fileUrl;
    return false;
  }

  qDebug() << "QUrl passed to loadFromYamlUrl:" << fileUrl;
  qDebug() << "Converted filePath from QUrl:" << filePath;
  loadFromYamlFile(filePath);
  return true;
}

bool FileController::loadFromYamlFile(const QString& filePath) {
  // Clear any previous error
  set_errorMessage({});

  // Use FileCoordinator to read the file - this triggers cloud file downloads
  // on MacOS (Dropbox, iCloud, etc.) via NSFileCoordinator
  QByteArray data;
  QString readError;
  if (!FileCoordinator::readFile(filePath, data, readError)) {
    qWarning() << "Failed to open file for reading:" << filePath << readError;
    set_errorMessage(tr("Could not open file: %1").arg(readError));
    return false;
  }

  // Check for empty file
  if (data.isEmpty()) {
    set_errorMessage(tr("File is empty: %1").arg(filePath));
    return false;
  }

  _budgetData.clear();

  // Track state from file for NavigationController
  // Default to current year/month if not specified in file
  QDate today = QDate::currentDate();
  int loadedTabIndex = 0;
  int loadedBudgetYear = today.year();
  int loadedBudgetMonth = today.month();
  int loadedAccountIdx = 0;
  int loadedCategoryIdx = 0;
  Operation* currentOperation = nullptr;

  try {
    ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(data.constData()));
    ryml::ConstNodeRef root = tree.crootref();

    // Load state section
    if (root.has_child("state")) {
      ryml::ConstNodeRef state = root["state"];
      if (state.has_child("currentTab")) {
        auto val = state["currentTab"].val();
        loadedTabIndex = QString::fromUtf8(val.str, val.len).toInt();
      }
      if (state.has_child("budgetYear")) {
        auto val = state["budgetYear"].val();
        loadedBudgetYear = QString::fromUtf8(val.str, val.len).toInt();
      }
      if (state.has_child("budgetMonth")) {
        auto val = state["budgetMonth"].val();
        loadedBudgetMonth = QString::fromUtf8(val.str, val.len).toInt();
      }
    }

    // Load categories
    if (root.has_child("categories")) {
      int catIdx = 0;
      for (ryml::ConstNodeRef cat : root["categories"]) {
        auto category = new Category();
        if (cat.has_child("name")) {
          auto val = cat["name"].val();
          category->set_name(QString::fromUtf8(val.str, val.len));
        }
        if (cat.has_child("budget_limit")) {
          auto val = cat["budget_limit"].val();
          category->set_budgetLimit(QString::fromUtf8(val.str, val.len).toDouble());
        }
        if (cat.has_child("current")) {
          auto val = cat["current"].val();
          if (QString::fromUtf8(val.str, val.len).toLower() == "true") {
            loadedCategoryIdx = catIdx;
          }
        }

        // Load leftover decisions
        if (cat.has_child("leftover_decisions")) {
          for (ryml::ConstNodeRef decisionNode : cat["leftover_decisions"]) {
            int year = 0, month = 0;
            LeftoverDecision decision;

            if (decisionNode.has_child("year")) {
              auto val = decisionNode["year"].val();
              year = QString::fromUtf8(val.str, val.len).toInt();
            }
            if (decisionNode.has_child("month")) {
              auto val = decisionNode["month"].val();
              month = QString::fromUtf8(val.str, val.len).toInt();
            }
            // New format: separate save_amount and report_amount
            if (decisionNode.has_child("save_amount")) {
              auto val = decisionNode["save_amount"].val();
              decision.saveAmount = QString::fromUtf8(val.str, val.len).toDouble();
            }
            if (decisionNode.has_child("report_amount")) {
              auto val = decisionNode["report_amount"].val();
              decision.reportAmount = QString::fromUtf8(val.str, val.len).toDouble();
            }
            // Legacy format: action + amount
            if (decisionNode.has_child("action") && decisionNode.has_child("amount")) {
              auto actionVal = decisionNode["action"].val();
              QString actionStr = QString::fromUtf8(actionVal.str, actionVal.len).toLower();
              auto amountVal = decisionNode["amount"].val();
              double amount = QString::fromUtf8(amountVal.str, amountVal.len).toDouble();
              if (actionStr == "save") {
                decision.saveAmount = amount;
              } else if (actionStr == "report") {
                decision.reportAmount = amount;
              }
            }

            if (year > 0 && month > 0 && !decision.isEmpty()) {
              category->setLeftoverDecision(year, month, decision);
            }
          }
        }

        _categoryController.addCategory(category);
        catIdx++;
      }
    }

    // Load accounts
    if (root.has_child("accounts")) {
      int accIdx = 0;
      for (ryml::ConstNodeRef acc : root["accounts"]) {
        auto account = new Account();
        if (acc.has_child("name")) {
          auto val = acc["name"].val();
          account->set_name(QString::fromUtf8(val.str, val.len));
        }
        if (acc.has_child("current")) {
          auto val = acc["current"].val();
          if (QString::fromUtf8(val.str, val.len).toLower() == "true") {
            loadedAccountIdx = accIdx;
          }
        }
        // Note: balance field is ignored - balance is calculated from operations
        if (acc.has_child("operations")) {
          for (ryml::ConstNodeRef opNode : acc["operations"]) {
            auto op = new Operation(account);
            if (opNode.has_child("date")) {
              auto val = opNode["date"].val();
              op->set_date(QDate::fromString(QString::fromUtf8(val.str, val.len), "yyyy-MM-dd"));
            }
            if (opNode.has_child("amount")) {
              auto val = opNode["amount"].val();
              op->set_amount(QString::fromUtf8(val.str, val.len).toDouble());
            }
            // Handle split operations (allocations) vs single category
            if (opNode.has_child("allocations")) {
              QList<CategoryAllocation> allocations;
              for (ryml::ConstNodeRef allocNode : opNode["allocations"]) {
                CategoryAllocation alloc;
                if (allocNode.has_child("category")) {
                  auto val = allocNode["category"].val();
                  alloc.category = _categoryController.getCategoryByName(QString::fromUtf8(val.str, val.len));
                }
                if (allocNode.has_child("amount")) {
                  auto val = allocNode["amount"].val();
                  alloc.amount = QString::fromUtf8(val.str, val.len).toDouble();
                }
                allocations.append(alloc);
              }
              op->setAllocations(allocations);
            } else if (opNode.has_child("category")) {
              auto val = opNode["category"].val();
              op->set_category(_categoryController.getCategoryByName(QString::fromUtf8(val.str, val.len)));
            }
            if (opNode.has_child("description")) {
              auto val = opNode["description"].val();
              op->set_description(QString::fromUtf8(val.str, val.len));
            }
            if (opNode.has_child("budget_date")) {
              auto val = opNode["budget_date"].val();
              op->set_budgetDate(QDate::fromString(QString::fromUtf8(val.str, val.len), "yyyy-MM-dd"));
            }
            if (opNode.has_child("current")) {
              auto val = opNode["current"].val();
              if (QString::fromUtf8(val.str, val.len).toLower() == "true") {
                // Set this operation as the current operation for this account
                account->set_currentOperation(op);
                // Also track for navigation if this is the current account
                if (accIdx == loadedAccountIdx) {
                  currentOperation = op;
                }
              }
            }
            account->appendOperation(op);  // Preserve file order
          }
        }
        _budgetData.addAccount(account);
        accIdx++;
      }
    }

    // Load categorization rules
    if (root.has_child("rules")) {
      _ruleController.clearRules();
      for (ryml::ConstNodeRef ruleNode : root["rules"]) {
        Category* category = nullptr;
        QString descriptionPrefix;

        if (ruleNode.has_child("category")) {
          auto val = ruleNode["category"].val();
          category = _categoryController.getCategoryByName(QString::fromUtf8(val.str, val.len));
        }
        if (ruleNode.has_child("description_prefix")) {
          auto val = ruleNode["description_prefix"].val();
          descriptionPrefix = QString::fromUtf8(val.str, val.len);
        }

        if (category && !descriptionPrefix.isEmpty()) {
          auto* rule = new CategorizationRule(category, descriptionPrefix);
          _ruleController.addRule(rule);
        }
      }
    }
  } catch (const std::exception& e) {
    qWarning() << "YAML parsing error:" << e.what();
    set_errorMessage(tr("Could not parse file: %1").arg(QString::fromUtf8(e.what())));
    return false;
  }

  // Refresh account model
  _budgetData.accountModel()->refresh();

  // Calculate operation index from pointer
  int loadedOperationIdx = 0;
  if (currentOperation) {
    QList<Account*> accs = _budgetData.accounts();
    int boundedAccountIdx = qBound(0, loadedAccountIdx, accs.size() - 1);
    Account* account = _budgetData.getAccount(boundedAccountIdx);
    if (account) {
      QList<Operation*> ops = account->operations();
      int idx = ops.indexOf(currentOperation);
      if (idx >= 0) {
        loadedOperationIdx = idx;
      }
    }
  }

  // Emit signal with loaded navigation state for NavigationController
  emit navigationStateLoaded(
      loadedTabIndex,
      loadedBudgetYear,
      loadedBudgetMonth,
      qBound(0, loadedAccountIdx, qMax(0, _budgetData.accountCount() - 1)),
      qBound(0, loadedCategoryIdx, qMax(0, _categoryController.categoryCount() - 1)),
      loadedOperationIdx);

  set_currentFilePath(filePath);
  _undoStack.clear();
  _undoStack.setClean();

  // Add to recent files
  _appSettings.addRecentFile(filePath);

  emit yamlFileLoaded();
  emit dataLoaded();
  qDebug() << "Budget data loaded from:" << filePath;
  qDebug() << "  Accounts:" << _budgetData.accountCount();
  qDebug() << "  Categories:" << _categoryController.categoryCount();

  return true;
}

bool FileController::importFromCsv(const QUrl& fileUrl,
                                   const QString& accountName,
                                   bool useCategories) {
  // Clear any previous error
  set_errorMessage({});

  QString filePath = fileUrl.toLocalFile();
  qDebug() << "QUrl passed to importFromCsv:" << fileUrl;
  qDebug() << "Converted filePath from QUrl:" << filePath;
  qDebug() << "  Use categories:" << useCategories;

  // First pass: detect delimiter from first line
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "Failed to open file:" << file.errorString();
    set_errorMessage(tr("Could not open file: %1").arg(file.errorString()));
    return false;
  }

  QTextStream firstPass(&file);
  QString headerLine = firstPass.readLine();
  file.close();

  qDebug() << "Header (raw):" << headerLine;

  // Auto-detect delimiter: if header contains semicolons, use semicolon; otherwise use comma
  QChar delimiter = headerLine.contains(';') ? ';' : ',';
  qDebug() << "Detected delimiter:" << delimiter;

  // Semicolon-separated files from French banks typically use Latin1
  // Re-open and re-read with correct encoding
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "Failed to reopen file:" << file.errorString();
    set_errorMessage(tr("Could not open file: %1").arg(file.errorString()));
    return false;
  }

  QTextStream in(&file);
  if (delimiter == ';') {
    in.setEncoding(QStringConverter::Latin1);
  }
  headerLine = in.readLine();
  qDebug() << "Header (decoded):" << headerLine;

  // Create or get account
  QString name = accountName.isEmpty() ? "Imported Account" : accountName;
  Account* account = _budgetData.getAccountByName(name);
  bool isNewAccount = false;
  if (!account) {
    // New account - will be added to BudgetData via AddAccountCommand when undo stack is pushed
    account = new Account(name);
    isNewAccount = true;
  }

  // Parse header to detect column indices
  QStringList headerFields = parseCsvLine(headerLine, delimiter);
  CsvFieldIndices idx = parseHeader(headerFields);

  // Log detected columns
  qDebug() << "Detected columns:";
  qDebug() << "  date:" << idx.date;
  qDebug() << "  budgetDate:" << idx.budgetDate;
  qDebug() << "  description:" << idx.description;
  qDebug() << "  category:" << idx.category;
  qDebug() << "  debit:" << idx.debit;
  qDebug() << "  credit:" << idx.credit;
  qDebug() << "  amount:" << idx.amount;

  // Validate required columns
  if (!idx.isValid()) {
    qDebug() << "Invalid CSV format: missing required columns (date, description, and debit/credit/amount)";
    qDebug() << "Available headers:";
    for (int i = 0; i < headerFields.size(); i++) {
      qDebug() << "  [" << i << "]" << headerFields[i];
    }
    file.close();
    set_errorMessage(tr("Invalid CSV format: missing required columns (date, description, and debit/credit/amount)"));
    return false;
  }

  // Create a macro command that composes all the sub-commands
  QUndoCommand* macroCommand = new QUndoCommand();

  QList<Operation*> importedOperations;
  int skippedCount = 0;
  QSet<Category*> newCategories;
  double totalBalance = 0.0;

  while (!in.atEnd()) {
    QString line = in.readLine();

    // Skip empty lines
    if (isEmptyLine(line, delimiter)) {
      continue;
    }

    QStringList fields = parseCsvLine(line, delimiter);

    // Parse date (required)
    QDate date = QDate::fromString(getField(fields, idx.date), "dd/MM/yyyy");
    if (!date.isValid()) {
      qDebug() << "Skipping row with invalid date:" << getField(fields, idx.date);
      skippedCount++;
      continue;
    }

    // Parse amount (required - from debit, credit, or amount column)
    double amount = 0.0;
    if (idx.amount >= 0) {
      QString amountStr = getField(fields, idx.amount);
      if (!amountStr.isEmpty()) {
        amount = parseAmount(amountStr);
      }
    } else {
      QString debitStr = getField(fields, idx.debit);
      QString creditStr = getField(fields, idx.credit);
      if (!debitStr.isEmpty()) {
        amount = parseAmount(debitStr);
      } else if (!creditStr.isEmpty()) {
        amount = parseAmount(creditStr);
      }
    }

    // Parse description (required)
    QString description = getField(fields, idx.description);
    if (description.isEmpty()) {
      qDebug() << "Skipping row with empty description";
      skippedCount++;
      continue;
    }

    // Skip duplicate operations
    if (account->hasOperation(date, amount, description)) {
      continue;
    }

    // Determine category based on useCategories flag
    Category* category = nullptr;
    if (useCategories) {
      // Use category from CSV (last matching category column = most specific)
      QString categoryName = getField(fields, idx.category);
      if (!categoryName.isEmpty()) {
        category = _categoryController.getCategoryByName(categoryName);
        if (category == nullptr) {
          for (auto cat : newCategories) {
            if (cat->name() == categoryName) {
              category = cat;
              break;
            }
          }
        }
        if (category == nullptr) {
          category = new Category(categoryName, 0.0);
          new AddCategoryCommand(&_categoryController, category, macroCommand);
          newCategories.insert(category);
        }
      }
    }
    // If useCategories is false or category is empty, leave category empty

    // Create operation
    Operation* operation = new Operation(account);
    operation->set_date(date);
    operation->set_amount(amount);
    operation->set_description(description);
    operation->set_category(category);

    // Parse budget date (optional - falls back to date if not set)
    if (idx.budgetDate >= 0) {
      QString budgetDateStr = getField(fields, idx.budgetDate);
      if (!budgetDateStr.isEmpty()) {
        QDate budgetDate = QDate::fromString(budgetDateStr, "dd/MM/yyyy");
        if (budgetDate.isValid()) {
          operation->set_budgetDate(budgetDate);
        }
      }
    }

    totalBalance += amount;

    importedOperations.append(operation);
  }

  file.close();

  qDebug() << "Import complete:";
  qDebug() << "  Imported:" << importedOperations.size() << "operations";
  qDebug() << "  Skipped:" << skippedCount << "rows";
  qDebug() << "  Total balance delta:" << totalBalance;
  if (useCategories) {
    qDebug() << "  New categories:" << newCategories.count();
  }

  // Add operations and categories via undo command (if any were imported)
  if (!importedOperations.isEmpty()) {
    // Add account command first (if new account)
    if (isNewAccount) {
      new AddAccountCommand(account, &_budgetData, macroCommand);
    }

    // Add operations command
    new ImportOperationsCommand(*account, *_budgetData.operationModel(), importedOperations, macroCommand);

    // Set text based on what was imported
    if (isNewAccount && newCategories.count()) {
      macroCommand->setText(QObject::tr("Import %n operation(s) to new account with %1 category(ies)", "", importedOperations.size())
                                .arg(newCategories.count()));
    } else if (isNewAccount) {
      macroCommand->setText(QObject::tr("Import %n operation(s) to new account", "", importedOperations.size()));
    } else if (newCategories.count()) {
      macroCommand->setText(QObject::tr("Import %n operation(s) with %1 category(ies)", "", importedOperations.size())
                                .arg(newCategories.count()));
    } else {
      macroCommand->setText(QObject::tr("Import %n operation(s)", "", importedOperations.size()));
    }

    _undoStack.push(macroCommand);
  } else {
    delete macroCommand;
    // No operations imported, clean up
    qDeleteAll(newCategories);
    if (isNewAccount) {
      delete account;
    }
  }

  // Set as current account
  QList<Account*> accs = _budgetData.accounts();
  int accountIndex = accs.indexOf(account);
  if (accountIndex >= 0) {
    _navController.set_currentAccountIndex(accountIndex);
  }

  // Apply categorization rules to imported operations
  for (Operation* op : importedOperations) {
    _ruleController.applyRulesToOperation(op);
  }

  _budgetData.accountModel()->refresh();

  // Select all imported operations
  if (!importedOperations.isEmpty()) {
    account->clearSelection();
    for (Operation* op : importedOperations) {
      account->select(op, true);  // Extend selection to include all imported operations
    }
    // Set the first imported operation as the current operation
    account->set_currentOperation(importedOperations.first());
  }

  emit dataLoaded();

  return true;
}

void FileController::clear() {
  _budgetData.clear();
  set_currentFilePath({});
  emit navigationStateLoaded(0, 0, 0, -1, -1, -1);
}

void FileController::loadInitialFile(const QStringList& args) {
  // Command line argument takes priority (skip first arg which is the program name)
  if (args.size() > 1) {
    QString filePath = args.at(1);
    if (filePath.endsWith(".comptine") || filePath.endsWith(".yaml") || filePath.endsWith(".yml")) {
      loadFromYamlFile(filePath);
      return;
    } else if (filePath.endsWith(".csv")) {
      importFromCsv(QUrl::fromLocalFile(filePath));
      return;
    }
  }

  // Fall back to most recent file
  QStringList recentFiles = _appSettings.recentFiles();
  if (!recentFiles.isEmpty()) {
    QString lastFile = recentFiles.first();
    if (QFile::exists(lastFile)) {
      loadFromYamlFile(lastFile);
    }
  }
}
