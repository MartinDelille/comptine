// Must be included before Qt headers to avoid 'emit' keyword conflict
#define RYML_NO_DEFAULT_CALLBACKS
#include <c4/format.hpp>
#include <ryml.hpp>
#include <ryml_std.hpp>

#include <QClipboard>
#include <QDate>
#include <QDebug>
#include <QFile>
#include <QGuiApplication>
#include <QTextStream>
#include "AccountListModel.h"
#include "BudgetData.h"
#include "CsvParser.h"
#include "OperationListModel.h"
#include "UndoCommands.h"

using namespace CsvParser;

BudgetData::BudgetData(QObject *parent) :
    QObject(parent),
    _operationModel(new OperationListModel(this)),
    _accountModel(new AccountListModel(this)),
    _undoStack(new QUndoStack(this)) {
  _accountModel->setAccounts(&_accounts);
}

BudgetData::~BudgetData() {
  clear();
}

int BudgetData::accountCount() const {
  return _accounts.size();
}

QList<Account *> BudgetData::accounts() const {
  return _accounts;
}

Account *BudgetData::getAccount(int index) const {
  if (index >= 0 && index < _accounts.size()) {
    return _accounts[index];
  }
  return nullptr;
}

Account *BudgetData::getAccountByName(const QString &name) const {
  for (Account *account : _accounts) {
    if (account->name() == name) {
      return account;
    }
  }
  return nullptr;
}

void BudgetData::renameCurrentAccount(const QString &newName) {
  Account *account = currentAccount();
  if (account && !newName.isEmpty() && account->name() != newName) {
    _undoStack->push(new RenameAccountCommand(account, _accountModel,
                                              account->name(), newName));
    emit currentAccountChanged();
  }
}

void BudgetData::addAccount(Account *account) {
  if (account) {
    account->setParent(this);
    _accounts.append(account);
    if (_currentAccountIndex < 0) {
      set_currentAccountIndex(0);
    }
    emit accountCountChanged();
  }
}

void BudgetData::removeAccount(int index) {
  if (index >= 0 && index < _accounts.size()) {
    delete _accounts.takeAt(index);
    if (_currentAccountIndex >= _accounts.size()) {
      set_currentAccountIndex(_accounts.size() - 1);
    }
    emit accountCountChanged();
  }
}

void BudgetData::clearAccounts() {
  _operationModel->setAccount(nullptr);
  _currentAccountIndex = -1;
  qDeleteAll(_accounts);
  _accounts.clear();
  _accountModel->refresh();
  emit accountCountChanged();
  emit currentAccountIndexChanged();
  emit currentAccountChanged();
}

Account *BudgetData::currentAccount() const {
  return getAccount(_currentAccountIndex);
}

int BudgetData::currentAccountIndex() const {
  return _currentAccountIndex;
}

void BudgetData::set_currentAccountIndex(int index) {
  if (index != _currentAccountIndex && index >= -1 && index < _accounts.size()) {
    _currentAccountIndex = index;
    _operationModel->setAccount(currentAccount());
    emit currentAccountIndexChanged();
    emit currentAccountChanged();
  }
}

int BudgetData::categoryCount() const {
  return _categories.size();
}

QList<Category *> BudgetData::categories() const {
  return _categories;
}

Category *BudgetData::getCategory(int index) const {
  if (index >= 0 && index < _categories.size()) {
    return _categories[index];
  }
  return nullptr;
}

Category *BudgetData::getCategoryByName(const QString &name) const {
  for (Category *category : _categories) {
    if (category->name() == name) {
      return category;
    }
  }
  return nullptr;
}

void BudgetData::addCategory(Category *category) {
  if (category) {
    category->setParent(this);
    _categories.append(category);
    emit categoryCountChanged();
  }
}

void BudgetData::removeCategory(int index) {
  if (index >= 0 && index < _categories.size()) {
    delete _categories.takeAt(index);
    emit categoryCountChanged();
  }
}

void BudgetData::editCategory(const QString &originalName, const QString &newName, double newBudgetLimit) {
  Category *category = getCategoryByName(originalName);
  if (!category) return;

  QString oldName = category->name();
  double oldBudgetLimit = category->budgetLimit();

  // Only create undo command if something changed
  if (oldName != newName || oldBudgetLimit != newBudgetLimit) {
    _undoStack->push(new EditCategoryCommand(category, this,
                                             oldName, newName,
                                             oldBudgetLimit, newBudgetLimit));
  }
}

void BudgetData::clearCategories() {
  qDeleteAll(_categories);
  _categories.clear();
  emit categoryCountChanged();
}

Category *BudgetData::takeCategoryByName(const QString &name) {
  for (int i = 0; i < _categories.size(); i++) {
    if (_categories[i]->name().compare(name, Qt::CaseInsensitive) == 0) {
      Category *cat = _categories.takeAt(i);
      cat->setParent(nullptr);  // Release Qt ownership
      emit categoryCountChanged();
      return cat;
    }
  }
  return nullptr;
}

QStringList BudgetData::categoryNames() const {
  QStringList names;
  for (const Category *category : _categories) {
    names.append(category->name());
  }
  names.sort(Qt::CaseInsensitive);
  return names;
}

void BudgetData::setOperationCategory(int operationIndex, const QString &newCategory) {
  Account *account = currentAccount();
  if (!account) return;

  Operation *operation = account->getOperation(operationIndex);
  if (!operation) return;

  QString oldCategory = operation->category();
  if (oldCategory != newCategory) {
    _undoStack->push(new SetOperationCategoryCommand(operation, _operationModel, this,
                                                     oldCategory, newCategory));
  }
}

void BudgetData::setOperationBudgetDate(int operationIndex, const QDate &newBudgetDate) {
  Account *account = currentAccount();
  if (!account) return;

  Operation *operation = account->getOperation(operationIndex);
  if (!operation) return;

  QDate oldBudgetDate = operation->budgetDate();
  if (oldBudgetDate != newBudgetDate) {
    _undoStack->push(new SetOperationBudgetDateCommand(operation, _operationModel, this,
                                                       oldBudgetDate, newBudgetDate));
  }
}

double BudgetData::spentInCategory(const QString &categoryName, int year, int month) const {
  double total = 0.0;
  for (const Account *account : _accounts) {
    for (const Operation *op : account->operations()) {
      // Use budgetDate for budget calculations (falls back to date if not set)
      QDate budgetDate = op->budgetDate();
      if (op->category() == categoryName && budgetDate.year() == year && budgetDate.month() == month) {
        total += op->amount();
      }
    }
  }
  return total;
}

QVariantList BudgetData::monthlyBudgetSummary(int year, int month) const {
  QVariantList result;
  for (const Category *category : _categories) {
    double total = spentInCategory(category->name(), year, month);
    double budgetLimit = category->budgetLimit();
    bool isIncome = budgetLimit > 0;

    // For display: show positive values
    // - Expenses: total is negative, we show as positive "spent"
    // - Income: total is positive, we show as positive "received"
    double displayAmount = isIncome ? total : -total;
    double displayLimit = std::abs(budgetLimit);
    double remaining = displayLimit - displayAmount;

    // Calculate percent used:
    // - For zero budget expense: any spending means exceeded (use infinity-like behavior)
    // - For zero budget income: no expectation yet
    double percentUsed;
    if (displayLimit > 0) {
      percentUsed = (displayAmount / displayLimit) * 100.0;
    } else if (!isIncome && displayAmount > 0) {
      // Zero expense budget with spending = exceeded (show as 100%+)
      percentUsed = 100.0 + displayAmount;  // Will show exceeded status
    } else {
      percentUsed = 0.0;
    }

    QVariantMap item;
    item["name"] = category->name();
    item["budgetLimit"] = displayLimit;
    item["signedBudgetLimit"] = budgetLimit;  // Original signed value for editing
    item["amount"] = displayAmount;
    item["remaining"] = remaining;
    item["percentUsed"] = percentUsed;
    item["isIncome"] = isIncome;
    result.append(item);
  }

  // Sort by category name
  std::sort(result.begin(), result.end(), [](const QVariant &a, const QVariant &b) {
    return a.toMap()["name"].toString().compare(b.toMap()["name"].toString(), Qt::CaseInsensitive) < 0;
  });

  return result;
}

QVariantList BudgetData::operationsForCategory(const QString &categoryName, int year, int month) const {
  QVariantList result;
  for (const Account *account : _accounts) {
    for (const Operation *op : account->operations()) {
      QDate budgetDate = op->budgetDate();
      if (op->category() == categoryName && budgetDate.year() == year && budgetDate.month() == month) {
        QVariantMap item;
        item["date"] = op->date();
        item["budgetDate"] = budgetDate;
        item["description"] = op->description();
        item["amount"] = op->amount();
        item["accountName"] = account->name();
        result.append(item);
      }
    }
  }

  // Sort by date (most recent first)
  std::sort(result.begin(), result.end(), [](const QVariant &a, const QVariant &b) {
    return a.toMap()["date"].toDate() > b.toMap()["date"].toDate();
  });

  return result;
}

void BudgetData::selectOperation(const QString &accountName, const QDate &date,
                                 const QString &description, double amount) {
  // Find the account index
  int accountIndex = -1;
  for (int i = 0; i < _accounts.size(); ++i) {
    if (_accounts[i]->name() == accountName) {
      accountIndex = i;
      break;
    }
  }

  if (accountIndex < 0) {
    return;
  }

  // Switch to the account
  set_currentAccountIndex(accountIndex);

  // Find the operation in the account
  Account *account = _accounts[accountIndex];
  const QList<Operation *> &ops = account->operations();
  for (int i = 0; i < ops.size(); ++i) {
    Operation *op = ops[i];
    if (op->date() == date && op->description() == description && qFuzzyCompare(op->amount(), amount)) {
      // Select this operation
      _operationModel->select(i);

      // Emit signal so OperationList can focus the operation
      emit operationSelected(i);

      // Switch to Operations tab
      set_currentTabIndex(0);
      return;
    }
  }
}

void BudgetData::clear() {
  clearAccounts();
  clearCategories();
  _undoStack->clear();
  _undoStack->setClean();
}

// Helper to convert QString to std::string for ryml
static std::string toStdString(const QString &s) {
  return s.toStdString();
}

bool BudgetData::saveToYaml(const QString &filePath) const {
  ryml::Tree tree;
  ryml::NodeRef root = tree.rootref();
  root |= ryml::MAP;

  // Write categories
  ryml::NodeRef categories = root["categories"];
  categories |= ryml::SEQ;
  for (const Category *category : _categories) {
    ryml::NodeRef cat = categories.append_child();
    cat |= ryml::MAP;
    cat["name"] << toStdString(category->name());
    cat["budget_limit"] << category->budgetLimit();
  }

  // Write accounts
  ryml::NodeRef accounts = root["accounts"];
  accounts |= ryml::SEQ;
  for (const Account *account : _accounts) {
    ryml::NodeRef acc = accounts.append_child();
    acc |= ryml::MAP;
    acc["name"] << toStdString(account->name());

    ryml::NodeRef operations = acc["operations"];
    operations |= ryml::SEQ;
    for (const Operation *op : account->operations()) {
      ryml::NodeRef opNode = operations.append_child();
      opNode |= ryml::MAP;
      opNode["date"] << toStdString(op->date().toString("yyyy-MM-dd"));
      opNode["amount"] << toStdString(QString::number(op->amount(), 'f', 2));
      opNode["category"] << toStdString(op->category());
      opNode["description"] << toStdString(op->description());
      // Only save budget_date if explicitly set (different from operation date)
      if (op->budgetDate() != op->date()) {
        opNode["budget_date"] << toStdString(op->budgetDate().toString("yyyy-MM-dd"));
      }
    }
  }

  // Emit YAML to string
  std::string yaml = ryml::emitrs_yaml<std::string>(tree);

  // Write to file
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    qWarning() << "Failed to open file for writing:" << filePath;
    return false;
  }

  QTextStream out(&file);
  out.setEncoding(QStringConverter::Utf8);
  out << QString::fromStdString(yaml);
  file.close();

  qDebug() << "Budget data saved to:" << filePath;
  _undoStack->setClean();
  emit const_cast<BudgetData *>(this)->dataSaved();
  return true;
}

bool BudgetData::loadFromYaml(const QString &filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "Failed to open file for reading:" << filePath;
    return false;
  }

  QByteArray data = file.readAll();
  file.close();

  clear();

  try {
    ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(data.constData()));
    ryml::ConstNodeRef root = tree.crootref();

    // Load categories
    if (root.has_child("categories")) {
      for (ryml::ConstNodeRef cat : root["categories"]) {
        auto category = new Category(this);
        if (cat.has_child("name")) {
          auto val = cat["name"].val();
          category->set_name(QString::fromUtf8(val.str, val.len));
        }
        if (cat.has_child("budget_limit")) {
          auto val = cat["budget_limit"].val();
          category->set_budgetLimit(QString::fromUtf8(val.str, val.len).toDouble());
        }
        _categories.append(category);
      }
    }

    // Load accounts
    if (root.has_child("accounts")) {
      for (ryml::ConstNodeRef acc : root["accounts"]) {
        auto account = new Account(this);
        if (acc.has_child("name")) {
          auto val = acc["name"].val();
          account->set_name(QString::fromUtf8(val.str, val.len));
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
            if (opNode.has_child("category")) {
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
            account->addOperation(op);
          }
        }
        _accounts.append(account);
      }
    }
  } catch (const std::exception &e) {
    qWarning() << "YAML parsing error:" << e.what();
    return false;
  }

  // Refresh account model first so QML bindings see the accounts
  _accountModel->refresh();
  emit accountCountChanged();
  emit categoryCountChanged();

  // Set first account as current
  if (!_accounts.isEmpty()) {
    set_currentAccountIndex(0);
  }

  set_currentFilePath(filePath);
  _undoStack->clear();
  _undoStack->setClean();
  emit yamlFileLoaded();
  emit dataLoaded();
  qDebug() << "Budget data loaded from:" << filePath;
  qDebug() << "  Accounts:" << _accounts.size();
  qDebug() << "  Categories:" << _categories.size();

  return true;
}

bool BudgetData::importFromCsv(const QString &filePath,
                               const QString &accountName,
                               bool useCategories) {
  qDebug() << "Importing CSV from:" << filePath;
  qDebug() << "  Use categories:" << useCategories;

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "Failed to open file:" << file.errorString();
    return false;
  }

  // Create or get account
  QString name = accountName.isEmpty() ? "Imported Account" : accountName;
  Account *account = getAccountByName(name);
  if (!account) {
    account = new Account(name, this);
    _accounts.append(account);
  }

  QTextStream in(&file);
  // Default to UTF-8 for reading header (most modern files use UTF-8)
  in.setEncoding(QStringConverter::Utf8);

  // Read header line to detect format
  if (in.atEnd()) {
    qDebug() << "Empty CSV file";
    file.close();
    return false;
  }

  QString headerLine = in.readLine();
  qDebug() << "Header:" << headerLine;

  // Auto-detect delimiter: if header contains semicolons, use semicolon; otherwise use comma
  QChar delimiter = headerLine.contains(';') ? ';' : ',';
  qDebug() << "Detected delimiter:" << delimiter;

  // Adjust encoding for remaining lines based on format
  // Semicolon-separated files from French banks typically use Latin1
  if (delimiter == ';') {
    in.setEncoding(QStringConverter::Latin1);
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
    return false;
  }

  QList<Operation *> importedOperations;
  int skippedCount = 0;
  double totalBalance = 0.0;

  // Build case-insensitive lookup for existing categories (only used if useCategories is true)
  QMap<QString, QString> existingCategoryLookup;  // lowercase -> actual name
  if (useCategories) {
    for (const Category *cat : _categories) {
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
      Category *cat = new Category(catName, 0.0, this);
      newCategories.append(cat);
    }
  }

  // Add operations and categories via undo command (if any were imported)
  if (!importedOperations.isEmpty()) {
    _undoStack->push(new ImportOperationsCommand(account, _operationModel, this,
                                                 importedOperations, newCategories));
  } else {
    // No operations imported, clean up categories
    qDeleteAll(newCategories);
  }

  // Set as current account
  int accountIndex = _accounts.indexOf(account);
  if (accountIndex >= 0) {
    set_currentAccountIndex(accountIndex);
  }

  emit accountCountChanged();
  _accountModel->refresh();

  // Select all imported operations
  if (!importedOperations.isEmpty()) {
    QSet<Operation *> importedSet(importedOperations.begin(), importedOperations.end());
    bool firstSelected = false;
    for (int i = 0; i < account->operationCount(); i++) {
      if (importedSet.contains(account->getOperation(i))) {
        _operationModel->select(i, firstSelected);  // First one clears, rest extend
        firstSelected = true;
      }
    }
  }

  emit dataLoaded();

  return true;
}

void BudgetData::copySelectedOperationsToClipboard() const {
  QString csv = _operationModel->selectedOperationsAsCsv();
  if (!csv.isEmpty()) {
    QGuiApplication::clipboard()->setText(csv);
  }
}

void BudgetData::undo() {
  _undoStack->undo();
}

void BudgetData::redo() {
  _undoStack->redo();
}
