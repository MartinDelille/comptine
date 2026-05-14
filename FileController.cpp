#include <yaml-cpp/yaml.h>

#include <QDate>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTextStream>
#include <QUrl>
#include <string>

#include "Account.h"
#include "AppSettings.h"
#include "BudgetData.h"
#include "Category.h"
#include "CategoryController.h"
#include "CsvParser.h"
#include "FileController.h"
#include "FileCoordinator.h"
#include "NavigationController.h"
#include "Operation.h"
#include "Rule.h"
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
  connect(&_fileWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString& path) {
    qDebug() << "File changed detected by QFileSystemWatcher:" << path;
    if (path == currentFilePath()) {
      if (hasUnsavedChanges()) {
        qDebug() << "Current file was modified externally, but there are unsaved changes.";
        emit externalChangeDetected();
      } else {
        qDebug() << "Current file was modified externally. Reloading...";
        reloadCurrentFile();
      }
    }
  });
}

bool FileController::hasUnsavedChanges() const {
  return !_undoStack.isClean();
}

// Helper to convert QString to std::string for yaml-cpp
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
  return saveToYamlFile(filePath);
}

bool FileController::saveToYamlFile(const QString& filePath) {
  // Clear any previous error
  set_errorMessage({});

  YAML::Emitter out;
  out << YAML::BeginMap;

  // Get navigation state from NavigationController
  int currentTabIndex = _navController.currentTabIndex();
  int currentAccountIndex = _navController.currentAccountIndex();
  int currentCategoryIndex = _navController.currentCategoryIndex();

  // Write state section
  out << YAML::Key << "state" << YAML::Value << YAML::BeginMap;
  out << YAML::Key << "currentTab" << YAML::Value << currentTabIndex;
  out << YAML::Key << "budgetDate" << YAML::Value << toStdString(_navController.budgetDate().toString("MMMM yyyy"));
  out << YAML::EndMap;

  // Write categories
  out << YAML::Key << "categories" << YAML::Value << YAML::BeginSeq;
  QList<Category*> cats = _categoryController.categories();
  for (int i = 0; i < cats.size(); i++) {
    const Category* category = cats[i];
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << toStdString(category->name());
    out << YAML::Key << "budget_limit" << YAML::Value << toStdString(QString::number(category->budgetLimit(), 'f', 2));
    if (i == currentCategoryIndex) {
      out << YAML::Key << "current" << YAML::Value << "true";
    }

    // Write month history (leftover decisions + budget limit overrides)
    QMap<YearMonth, MonthRecord> history = category->allMonthHistory();
    if (!history.isEmpty()) {
      out << YAML::Key << "month_history" << YAML::Value << YAML::BeginSeq;
      for (auto it = history.constBegin(); it != history.constEnd(); ++it) {
        const YearMonth& ym = it.key();
        const MonthRecord& record = it.value();
        if (!record.isEmpty()) {
          out << YAML::BeginMap;
          out << YAML::Key << "year" << YAML::Value << ym.year;
          out << YAML::Key << "month" << YAML::Value << ym.month;
          if (record.budgetLimit.has_value()) {
            out << YAML::Key << "budget_limit" << YAML::Value << toStdString(QString::number(record.budgetLimit.value(), 'f', 2));
          }
          if (record.saveAmount != 0.0) {
            out << YAML::Key << "save_amount" << YAML::Value << toStdString(QString::number(record.saveAmount, 'f', 2));
          }
          if (record.reportAmount != 0.0) {
            out << YAML::Key << "report_amount" << YAML::Value << toStdString(QString::number(record.reportAmount, 'f', 2));
          }
          out << YAML::EndMap;
        }
      }
      out << YAML::EndSeq;
    }

    out << YAML::EndMap;
  }
  out << YAML::EndSeq;

  // Write accounts
  out << YAML::Key << "accounts" << YAML::Value << YAML::BeginSeq;
  QList<Account*> accs = _budgetData.accounts();
  for (int accIdx = 0; accIdx < accs.size(); accIdx++) {
    const Account* account = accs[accIdx];
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << toStdString(account->name());
    if (accIdx == currentAccountIndex) {
      out << YAML::Key << "current" << YAML::Value << "true";
    }

    // Save import sources (filenames previously imported into this account)
    if (!account->importSourcePrefixes().isEmpty()) {
      out << YAML::Key << "import_source_prefixes" << YAML::Value << YAML::BeginSeq;
      for (const QString& source : account->importSourcePrefixes()) {
        out << toStdString(source);
      }
      out << YAML::EndSeq;
    }

    out << YAML::Key << "operations" << YAML::Value << YAML::BeginSeq;
    const auto& ops = account->operations();
    for (int opIdx = 0; opIdx < ops.size(); opIdx++) {
      const Operation* op = ops[opIdx];
      out << YAML::BeginMap;
      out << YAML::Key << "date" << YAML::Value << toStdString(op->date().toString("yyyy-MM-dd"));
      out << YAML::Key << "amount" << YAML::Value << toStdString(QString::number(op->amount(), 'f', 2));
      out << YAML::Key << "label" << YAML::Value << toStdString(op->label());

      // Handle split operations
      out << YAML::Key << "allocations" << YAML::Value << YAML::BeginSeq;
      for (const auto& alloc : op->allocations()) {
        out << YAML::BeginMap;
        if (alloc->category()) {
          out << YAML::Key << "category" << YAML::Value << toStdString(alloc->category()->name());
        }
        out << YAML::Key << "amount" << YAML::Value << toStdString(QString::number(alloc->amount(), 'f', 2));
        out << YAML::EndMap;
      }
      out << YAML::EndSeq;

      // Only save budget_date if explicitly set (different from operation date)
      if (op->budgetDate() != op->date()) {
        out << YAML::Key << "budget_date" << YAML::Value << toStdString(op->budgetDate().toString("yyyy-MM-dd"));
      }
      // Mark current operation for this account
      if (op == account->currentOperation()) {
        out << YAML::Key << "current" << YAML::Value << "true";
      }

      out << YAML::EndMap;
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;
  }
  out << YAML::EndSeq;

  // Write categorization rules
  QList<Rule*> rulesList = _ruleController.rules();
  if (!rulesList.isEmpty()) {
    out << YAML::Key << "rules" << YAML::Value << YAML::BeginSeq;
    for (const Rule* rule : rulesList) {
      out << YAML::BeginMap;
      if (rule->category()) {
        out << YAML::Key << "category" << YAML::Value << toStdString(rule->category()->name());
        out << YAML::Key << "label_prefix" << YAML::Value << toStdString(rule->labelPrefix());
        if (rule->amountFilter() != 0) {
          out << YAML::Key << "amount" << YAML::Value << rule->amountFilter();
        }
      }
      out << YAML::EndMap;
    }
    out << YAML::EndSeq;
  }

  out << YAML::EndMap;

  // Write to file
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    qWarning() << "Failed to open file for writing:" << filePath;
    set_errorMessage(tr("Could not save file: %1").arg(file.errorString()));
    return false;
  }

  QTextStream stream(&file);
  stream.setEncoding(QStringConverter::Utf8);
  stream << QString::fromStdString(std::string(out.c_str()));
  stream << "\n";
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
  return loadFromYamlFile(filePath);
}

// Helper to safely read a string value from a YAML node
static QString yamlString(const YAML::Node& node) {
  if (!node.IsDefined() || node.IsNull()) {
    return {};
  }
  return QString::fromStdString(node.as<std::string>());
}

bool FileController::loadFromYamlFile(const QString& filePath) {
  clear();
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
  int loadedTabIndex = 0;
  QDate loadedBudgetDate = QDate::currentDate();
  int loadedAccountIdx = 0;
  int loadedCategoryIdx = 0;
  Operation* currentOperation = nullptr;

  try {
    YAML::Node root = YAML::Load(std::string(data.constData(), data.size()));

    // Load state section
    if (root["state"]) {
      YAML::Node state = root["state"];
      if (state["currentTab"]) {
        loadedTabIndex = state["currentTab"].as<int>();
      }
      if (state["budgetDate"]) {
        loadedBudgetDate = QDate::fromString(yamlString(state["budgetDate"]), "MMMM yyyy");
      } else {
        int loadedBudgetYear = 0;
        int loadedBudgetMonth = 0;
        if (state["budgetYear"]) {
          loadedBudgetYear = state["budgetYear"].as<int>();
        }
        if (state["budgetMonth"]) {
          loadedBudgetMonth = state["budgetMonth"].as<int>();
        }
        if (loadedBudgetYear > 0 && loadedBudgetMonth > 0) {
          loadedBudgetDate = QDate(loadedBudgetYear, loadedBudgetMonth, 1);
        }
      }
    }

    // Load categories
    if (root["categories"]) {
      int catIdx = 0;
      for (const auto& cat : root["categories"]) {
        auto category = new Category();
        if (cat["name"]) {
          category->set_name(yamlString(cat["name"]));
        }
        if (cat["budget_limit"]) {
          category->set_budgetLimit(yamlString(cat["budget_limit"]).toDouble());
        }
        if (cat["current"]) {
          if (yamlString(cat["current"]).toLower() == "true") {
            loadedCategoryIdx = catIdx;
          }
        }

        // Load month history (new format) or leftover decisions (legacy format)
        auto loadMonthEntries = [&](const YAML::Node& entriesNode) {
          for (const auto& entryNode : entriesNode) {
            int year = 0, month = 0;
            MonthRecord record;

            if (entryNode["year"]) {
              year = entryNode["year"].as<int>();
            }
            if (entryNode["month"]) {
              month = entryNode["month"].as<int>();
            }
            // Budget limit override (new in month_history format)
            if (entryNode["budget_limit"]) {
              record.budgetLimit = yamlString(entryNode["budget_limit"]).toDouble();
            }
            // New format: separate save_amount and report_amount
            if (entryNode["save_amount"]) {
              record.saveAmount = yamlString(entryNode["save_amount"]).toDouble();
            }
            if (entryNode["report_amount"]) {
              record.reportAmount = yamlString(entryNode["report_amount"]).toDouble();
            }
            // Legacy format: action + amount
            if (entryNode["action"] && entryNode["amount"]) {
              QString actionStr = yamlString(entryNode["action"]).toLower();
              double amount = yamlString(entryNode["amount"]).toDouble();
              if (actionStr == "save") {
                record.saveAmount = amount;
              } else if (actionStr == "report") {
                record.reportAmount = amount;
              }
            }

            if (year > 0 && month > 0 && !record.isEmpty()) {
              category->setMonthRecord(year, month, record);
            }
          }
        };

        if (cat["month_history"]) {
          loadMonthEntries(cat["month_history"]);
        } else if (cat["leftover_decisions"]) {
          loadMonthEntries(cat["leftover_decisions"]);
        }

        _categoryController.addCategory(category);
        catIdx++;
      }
    }

    // Load accounts
    if (root["accounts"]) {
      int accIdx = 0;
      for (const auto& acc : root["accounts"]) {
        auto account = new Account();
        if (acc["name"]) {
          account->set_name(yamlString(acc["name"]));
        }
        if (acc["current"]) {
          if (yamlString(acc["current"]).toLower() == "true") {
            loadedAccountIdx = accIdx;
          }
        }
        // Load import source prefix
        YAML::Node importSourcePrefixes;
        if (acc["import_source_prefixes"]) {
          importSourcePrefixes = acc["import_source_prefixes"];
        } else if (acc["import_sources"]) {  // legacy support
          importSourcePrefixes = acc["import_sources"];
        }
        if (importSourcePrefixes) {
          QStringList sources;
          for (const auto& sourceNode : importSourcePrefixes) {
            sources.append(QString::fromStdString(sourceNode.as<std::string>()));
          }
          account->setImportSourcePrefixes(sources);
        }
        // Note: balance field is ignored - balance is calculated from operations
        if (acc["operations"]) {
          for (const auto& opNode : acc["operations"]) {
            auto op = new Operation(account);
            if (opNode["date"]) {
              op->set_date(QDate::fromString(yamlString(opNode["date"]), "yyyy-MM-dd"));
            }
            if (opNode["amount"]) {
              op->set_amount(yamlString(opNode["amount"]).toDouble());
            }
            // Handle split operations (allocations) vs single category
            if (opNode["allocations"]) {
              QList<Allocation*> allocations;
              for (const auto& allocNode : opNode["allocations"]) {
                if (allocNode["category"] && allocNode["amount"]) {
                  allocations.append(new Allocation(
                      _categoryController.getCategoryByName(yamlString(allocNode["category"])),
                      yamlString(allocNode["amount"]).toDouble()));
                }
              }
              op->setAllocations(allocations);
            } else if (opNode["category"]) {  // Support for old format (<= 0.14)
              auto category = _categoryController.getCategoryByName(yamlString(opNode["category"]));
              op->setAllocations({ new Allocation(category, op->amount()) });
            }

            if (opNode["label"]) {
              op->set_label(yamlString(opNode["label"]));
            } else if (opNode["description"]) {
              op->set_label(yamlString(opNode["description"]));
            }
            if (opNode["details"]) {
              op->set_details(yamlString(opNode["details"]));
            }
            if (opNode["budget_date"]) {
              op->set_budgetDate(QDate::fromString(yamlString(opNode["budget_date"]), "yyyy-MM-dd"));
            }
            if (opNode["current"]) {
              if (yamlString(opNode["current"]).toLower() == "true") {
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
    if (root["rules"]) {
      _ruleController.clearRules();
      for (const auto& ruleNode : root["rules"]) {
        Category* category = nullptr;
        QString labelPrefix;

        if (ruleNode["category"]) {
          category = _categoryController.getCategoryByName(yamlString(ruleNode["category"]));
        }
        if (ruleNode["label_prefix"]) {
          labelPrefix = yamlString(ruleNode["label_prefix"]);
        }

        if (category && !labelPrefix.isEmpty()) {
          Rule* rule;
          if (ruleNode["amount"]) {
            double amount = ruleNode["amount"].as<double>();
            rule = new Rule(category, labelPrefix, amount);
          } else {
            rule = new Rule(category, labelPrefix);
          }
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
    Account* account = _budgetData.accountAt(boundedAccountIdx);
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
      loadedBudgetDate,
      qBound(0, loadedAccountIdx, qMax(0, _budgetData.accountCount() - 1)),
      qBound(0, loadedCategoryIdx, qMax(0, _categoryController.rowCount() - 1)),
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
  qDebug() << "  Categories:" << _categoryController.rowCount();

  _fileWatcher.addPath(filePath);

  return true;
}

void FileController::reloadCurrentFile() {
  if (!currentFilePath().isEmpty()) {
    int tabIndex = _navController.currentTabIndex();
    QDate budgetDate = _navController.budgetDate();
    int accountIndex = _navController.currentAccountIndex();
    int categoryIndex = _navController.currentCategoryIndex();
    Account* currentAccount = _budgetData.accountAt(accountIndex);
    int operationIndex = currentAccount ? currentAccount->operations().indexOf(currentAccount->currentOperation()) : 0;
    loadFromYamlFile(currentFilePath());
    emit navigationStateLoaded(tabIndex, budgetDate, accountIndex, categoryIndex, operationIndex);
  }
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

  QByteArray bytes = file.readAll();
  QStringConverter::Encoding encoding = QStringConverter::Utf8;  // Default

  if (bytes.startsWith("\xEF\xBB\xBF")) {
    encoding = QStringConverter::Utf8;  // UTF-8 BOM
  } else if (bytes.startsWith("\xFF\xFE")) {
    encoding = QStringConverter::Utf16LE;  // UTF-16 LE BOM
  } else if (bytes.startsWith("\xFE\xFF")) {
    encoding = QStringConverter::Utf16BE;  // UTF-16 BE BOM
  } else {
    // No BOM detected; could prompt user or use default

    // Try to decode as UTF-8
    QString utf8Text = QString::fromUtf8(bytes);
    if (utf8Text.contains(QChar::ReplacementCharacter)) {
      // Invalid UTF-8 sequence detected, likely Latin1
      encoding = QStringConverter::Latin1;
    } else {
      encoding = QStringConverter::Utf8;
    }
  }

  QStringList headerFields;
  QChar delimiter = ';';

  QTextStream in(bytes);
  in.setEncoding(encoding);
  CsvFieldIndices idx;

  while (!in.atEnd() && !idx.isValid()) {
    QString headerLine = in.readLine();
    qDebug() << "Header (decoded):" << headerLine;

    // Parse header to detect column indices
    delimiter = ';';
    headerFields = parseCsvLine(headerLine, delimiter);
    idx = parseHeader(headerFields);

    if (!idx.isValid()) {
      delimiter = ',';
      headerFields = parseCsvLine(headerLine, delimiter);
      idx = parseHeader(headerFields);
    }
  }

  // Log detected columns
  qDebug() << "Detected columns:";
  qDebug() << "  date:" << idx.date;
  qDebug() << "  budgetDate:" << idx.budgetDate;
  qDebug() << "  label:" << idx.label;
  qDebug() << "  details:" << idx.details;
  qDebug() << "  category:" << idx.category;
  qDebug() << "  debit:" << idx.debit;
  qDebug() << "  credit:" << idx.credit;
  qDebug() << "  amount:" << idx.amount;

  if (!idx.isValid()) {
    qDebug() << "Invalid CSV format: missing required columns (date, label, and debit/credit/amount)";
    qDebug() << "Available headers:";
    for (int i = 0; i < headerFields.size(); i++) {
      qDebug() << "  [" << i << "]" << headerFields[i];
    }
    file.close();
    set_errorMessage(tr("Invalid CSV format: missing required columns (date, label, and debit/credit/amount)"));
    return false;
  }

  // Create or get account
  QString name = accountName.isEmpty() ? "Imported Account" : accountName;
  Account* account = _budgetData.accountByName(name);
  bool isNewAccount = false;
  if (!account) {
    // New account - will be added to BudgetData via AddAccountCommand when undo stack is pushed
    account = new Account(name);
    isNewAccount = true;
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
      date = QDate::fromString(getField(fields, idx.date), "yyyy-MM-dd");
    }
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

    // Parse label (required)
    QString label = getField(fields, idx.label);
    if (label.isEmpty()) {
      qDebug() << "Skipping row with empty label";
      skippedCount++;
      continue;
    }

    // Skip duplicate operations
    if (account->hasOperation(date, amount, label)) {
      continue;
    }

    // Parse details
    QString details = getField(fields, idx.details);

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
    operation->set_label(label);
    operation->set_details(details);
    if (category) {
      operation->setAllocations({ new Allocation(category, amount) });
    }

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

  // Record import source for future auto-suggestion
  QString baseFilename = QFileInfo(fileUrl.toLocalFile()).fileName();
  account->addImportSourcePrefix(baseFilename);

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
  _categoryController.clear();
  if (_fileWatcher.files().contains(currentFilePath())) {
    _fileWatcher.removePath(currentFilePath());
  }
  set_currentFilePath({});
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
