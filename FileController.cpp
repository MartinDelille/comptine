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
#include "Category.h"
#include "CategoryController.h"
#include "CsvParser.h"
#include "FileController.h"
#include "NavigationController.h"
#include "Operation.h"
#include "UndoCommands.h"

using namespace CsvParser;

FileController::FileController(QObject *parent) : QObject(parent) {
}

void FileController::setAppSettings(AppSettings *settings) {
  _appSettings = settings;
}

void FileController::setBudgetData(BudgetData *budgetData) {
  _budgetData = budgetData;
}

void FileController::setCategoryController(CategoryController *categoryController) {
  _categoryController = categoryController;
}

void FileController::setNavigationController(NavigationController *navController) {
  _navController = navController;
}

bool FileController::hasUnsavedChanges() const {
  return _budgetData ? !_budgetData->undoStack()->isClean() : false;
}

// Helper to convert QString to std::string for ryml
static std::string toStdString(const QString &s) {
  return s.toStdString();
}

bool FileController::saveToYaml(const QString &filePath) {
  if (!_budgetData) return false;

  // Clear any previous error
  set_errorMessage({});

  ryml::Tree tree;
  ryml::NodeRef root = tree.rootref();
  root |= ryml::MAP;

  // Get navigation state from NavigationController
  int currentTabIndex = _navController ? _navController->currentTabIndex() : 0;
  int budgetYear = _navController ? _navController->budgetYear() : 0;
  int budgetMonth = _navController ? _navController->budgetMonth() : 0;
  int currentAccountIndex = _navController ? _navController->currentAccountIndex() : 0;
  int currentCategoryIndex = _navController ? _navController->currentCategoryIndex() : 0;
  int currentOperationIndex = _navController ? _navController->currentOperationIndex() : 0;

  // Write state section
  ryml::NodeRef state = root["state"];
  state |= ryml::MAP;
  state["currentTab"] << currentTabIndex;
  state["budgetYear"] << budgetYear;
  state["budgetMonth"] << budgetMonth;

  // Write categories
  ryml::NodeRef categories = root["categories"];
  categories |= ryml::SEQ;
  QList<Category *> cats = _categoryController ? _categoryController->categories() : QList<Category *>();
  for (int i = 0; i < cats.size(); i++) {
    const Category *category = cats[i];
    ryml::NodeRef cat = categories.append_child();
    cat |= ryml::MAP;
    cat["name"] << toStdString(category->name());
    cat["budget_limit"] << toStdString(QString::number(category->budgetLimit(), 'f', 2));
    if (i == currentCategoryIndex) {
      cat["current"] << "true";
    }
  }

  // Write accounts
  ryml::NodeRef accounts = root["accounts"];
  accounts |= ryml::SEQ;
  QList<Account *> accs = _budgetData->accounts();
  for (int accIdx = 0; accIdx < accs.size(); accIdx++) {
    const Account *account = accs[accIdx];
    ryml::NodeRef acc = accounts.append_child();
    acc |= ryml::MAP;
    acc["name"] << toStdString(account->name());
    if (accIdx == currentAccountIndex) {
      acc["current"] << "true";
    }

    ryml::NodeRef operations = acc["operations"];
    operations |= ryml::SEQ;
    const auto &ops = account->operations();
    for (int opIdx = 0; opIdx < ops.size(); opIdx++) {
      const Operation *op = ops[opIdx];
      ryml::NodeRef opNode = operations.append_child();
      opNode |= ryml::MAP;
      opNode["date"] << toStdString(op->date().toString("yyyy-MM-dd"));
      opNode["amount"] << toStdString(QString::number(op->amount(), 'f', 2));
      opNode["description"] << toStdString(op->description());

      // Handle split operations vs single category
      if (op->isSplit()) {
        ryml::NodeRef allocsNode = opNode["allocations"];
        allocsNode |= ryml::SEQ;
        for (const auto &alloc : op->allocationsList()) {
          ryml::NodeRef allocNode = allocsNode.append_child();
          allocNode |= ryml::MAP;
          allocNode["category"] << toStdString(alloc.category);
          allocNode["amount"] << toStdString(QString::number(alloc.amount, 'f', 2));
        }
      } else {
        opNode["category"] << toStdString(op->category());
      }

      // Only save budget_date if explicitly set (different from operation date)
      if (op->budgetDate() != op->date()) {
        opNode["budget_date"] << toStdString(op->budgetDate().toString("yyyy-MM-dd"));
      }
      // Mark current operation (only for current account)
      if (accIdx == currentAccountIndex && opIdx == currentOperationIndex) {
        opNode["current"] << "true";
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
  _budgetData->undoStack()->setClean();
  emit dataSaved();
  return true;
}

bool FileController::loadFromYaml(const QString &filePath) {
  if (!_budgetData || !_categoryController) return false;

  // Clear any previous error
  set_errorMessage({});

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "Failed to open file for reading:" << filePath;
    set_errorMessage(tr("Could not open file: %1").arg(file.errorString()));
    return false;
  }

  QByteArray data = file.readAll();
  file.close();

  _budgetData->clear();

  // Track state from file for NavigationController
  int loadedTabIndex = 0;
  int loadedBudgetYear = 0;
  int loadedBudgetMonth = 0;
  int loadedAccountIdx = 0;
  int loadedCategoryIdx = 0;
  Operation *currentOperation = nullptr;

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
        _categoryController->addCategory(category);
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
          int opIdx = 0;
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
                  alloc.category = QString::fromUtf8(val.str, val.len);
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
              op->set_category(QString::fromUtf8(val.str, val.len));
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
                // Only track current operation for the current account
                if (accIdx == loadedAccountIdx) {
                  currentOperation = op;
                }
              }
            }
            account->addOperation(op);
            opIdx++;
          }
        }
        _budgetData->addAccount(account);
        accIdx++;
      }
    }
  } catch (const std::exception &e) {
    qWarning() << "YAML parsing error:" << e.what();
    set_errorMessage(tr("Could not parse file: %1").arg(QString::fromUtf8(e.what())));
    return false;
  }

  // Refresh account model
  _budgetData->accountModel()->refresh();

  // Calculate operation index from pointer
  int loadedOperationIdx = 0;
  if (currentOperation) {
    QList<Account *> accs = _budgetData->accounts();
    int boundedAccountIdx = qBound(0, loadedAccountIdx, accs.size() - 1);
    Account *account = _budgetData->getAccount(boundedAccountIdx);
    if (account) {
      QList<Operation *> ops = account->operations();
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
      qBound(0, loadedAccountIdx, qMax(0, _budgetData->accountCount() - 1)),
      qBound(0, loadedCategoryIdx, qMax(0, _categoryController->categoryCount() - 1)),
      loadedOperationIdx);

  set_currentFilePath(filePath);
  _budgetData->undoStack()->clear();
  _budgetData->undoStack()->setClean();

  // Add to recent files
  if (_appSettings) {
    _appSettings->addRecentFile(filePath);
  }

  emit yamlFileLoaded();
  emit dataLoaded();
  qDebug() << "Budget data loaded from:" << filePath;
  qDebug() << "  Accounts:" << _budgetData->accountCount();
  qDebug() << "  Categories:" << _categoryController->categoryCount();

  return true;
}

bool FileController::importFromCsv(const QString &filePath,
                                   const QString &accountName,
                                   bool useCategories) {
  if (!_budgetData || !_categoryController) return false;

  // Clear any previous error
  set_errorMessage({});

  qDebug() << "Importing CSV from:" << filePath;
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
  Account *account = _budgetData->getAccountByName(name);
  if (!account) {
    account = new Account(name);
    _budgetData->addAccount(account);
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

  QList<Operation *> importedOperations;
  int skippedCount = 0;
  double totalBalance = 0.0;

  // Build case-insensitive lookup for existing categories (only used if useCategories is true)
  QMap<QString, QString> existingCategoryLookup;  // lowercase -> actual name
  if (useCategories) {
    for (const Category *cat : _categoryController->categories()) {
      existingCategoryLookup.insert(cat->name().toLower(), cat->name());
    }
  }

  // Track which new categories need to be created
  QSet<QString> newCategoryNames;

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
    QString category;
    if (useCategories) {
      // Use category from CSV (last matching category column = most specific)
      category = getField(fields, idx.category);

      // Track new categories that will need to be created
      if (!category.isEmpty() && !existingCategoryLookup.contains(category.toLower())) {
        newCategoryNames.insert(category);
        existingCategoryLookup.insert(category.toLower(), category);
      }
    }
    // If useCategories is false or category is empty, leave category empty

    // Create operation
    Operation *operation = new Operation(account);
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
    qDebug() << "  New categories:" << newCategoryNames.size();
  }

  // Create new category objects (only if useCategories is true)
  QList<Category *> newCategories;
  if (useCategories) {
    for (const QString &catName : newCategoryNames) {
      Category *cat = new Category(catName, 0.0);
      newCategories.append(cat);
    }
  }

  // Add operations and categories via undo command (if any were imported)
  if (!importedOperations.isEmpty()) {
    _budgetData->undoStack()->push(new ImportOperationsCommand(account, _budgetData->operationModel(), _categoryController,
                                                               importedOperations, newCategories));
  } else {
    // No operations imported, clean up categories
    qDeleteAll(newCategories);
  }

  // Set as current account
  QList<Account *> accs = _budgetData->accounts();
  int accountIndex = accs.indexOf(account);
  if (accountIndex >= 0 && _navController) {
    _navController->set_currentAccountIndex(accountIndex);
  }

  _budgetData->accountModel()->refresh();

  // Select all imported operations
  if (!importedOperations.isEmpty()) {
    QSet<Operation *> importedSet(importedOperations.begin(), importedOperations.end());
    bool firstSelected = false;
    for (int i = 0; i < account->operationCount(); i++) {
      if (importedSet.contains(account->getOperation(i))) {
        _budgetData->operationModel()->select(i, firstSelected);  // First one clears, rest extend
        firstSelected = true;
      }
    }
  }

  emit dataLoaded();

  return true;
}

void FileController::clear() {
  if (_budgetData) {
    _budgetData->clear();
  }
  if (_categoryController) {
    _categoryController->clearCategories();
  }
  set_currentFilePath({});
}
