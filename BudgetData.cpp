// Must be included before Qt headers to avoid 'emit' keyword conflict
#define RYML_NO_DEFAULT_CALLBACKS
#include <ryml.hpp>
#include <ryml_std.hpp>
#include <c4/format.hpp>

#include "BudgetData.h"
#include "CsvParser.h"
#include <QClipboard>
#include <QDate>
#include <QDebug>
#include <QFile>
#include <QGuiApplication>
#include <QTextStream>

using namespace CsvParser;

BudgetData::BudgetData(QObject *parent)
    : QObject(parent) {
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
  qDeleteAll(_accounts);
  _accounts.clear();
  _currentAccountIndex = -1;
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
    clearSelection(); // Clear selection when switching accounts
    emit currentAccountIndexChanged();
    emit currentAccountChanged();
    emit operationCountChanged();
  }
}

int BudgetData::operationCount() const {
  Account *account = currentAccount();
  return account ? account->operationCount() : 0;
}

Operation *BudgetData::getOperation(int index) const {
  Account *account = currentAccount();
  return account ? account->getOperation(index) : nullptr;
}

double BudgetData::balanceAtIndex(int index) const {
  Account *account = currentAccount();
  if (!account || index < 0 || index >= account->operationCount()) {
    return 0.0;
  }

  // Operations are sorted from most recent (index 0) to oldest (index n-1)
  // Balance = sum of all operations from oldest up to and including this one
  double balance = 0.0;
  for (int i = account->operationCount() - 1; i >= index; --i) {
    Operation *op = account->getOperation(i);
    if (op) {
      balance += op->amount();
    }
  }
  return balance;
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

void BudgetData::clearCategories() {
  qDeleteAll(_categories);
  _categories.clear();
  emit categoryCountChanged();
}

double BudgetData::spentInCategory(const QString &categoryName, int year, int month) const {
  double spent = 0.0;
  for (const Account *account : _accounts) {
    for (const Operation *op : account->operations()) {
      if (op->category() == categoryName &&
          op->date().year() == year &&
          op->date().month() == month &&
          op->amount() < 0) {
        spent += op->amount(); // Already negative
      }
    }
  }
  return spent;
}

QVariantList BudgetData::monthlyBudgetSummary(int year, int month) const {
  QVariantList result;
  for (const Category *category : _categories) {
    double spent = -spentInCategory(category->name(), year, month); // Make positive
    double budgetLimit = category->budgetLimit();
    double remaining = budgetLimit - spent;
    double percentUsed = budgetLimit > 0 ? (spent / budgetLimit) * 100.0 : 0.0;

    QVariantMap item;
    item["name"] = category->name();
    item["budgetLimit"] = budgetLimit;
    item["spent"] = spent;
    item["remaining"] = remaining;
    item["percentUsed"] = percentUsed;
    result.append(item);
  }
  return result;
}

void BudgetData::clear() {
  clearAccounts();
  clearCategories();
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
      opNode["amount"] << op->amount();
      opNode["category"] << toStdString(op->category());
      opNode["description"] << toStdString(op->description());
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

  // Set first account as current
  if (!_accounts.isEmpty()) {
    set_currentAccountIndex(0);
  }

  set_currentFilePath(filePath);
  emit accountCountChanged();
  emit categoryCountChanged();
  emit dataLoaded();
  qDebug() << "Budget data loaded from:" << filePath;
  qDebug() << "  Accounts:" << _accounts.size();
  qDebug() << "  Categories:" << _categories.size();

  return true;
}

bool BudgetData::importFromCsv(const QString &filePath, const QString &accountName) {
  qDebug() << "Importing CSV from:" << filePath;

  // Clear selection when importing new data
  clearSelection();

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
  qDebug() << "  description:" << idx.description;
  qDebug() << "  category:" << idx.category;
  qDebug() << "  subCategory:" << idx.subCategory;
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

  int importCount = 0;
  int skippedCount = 0;
  double totalBalance = 0.0;
  Operation *lastImportedOperation = nullptr;

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
      qDebug() << "  Amount string from Montant column:" << amountStr;
      if (!amountStr.isEmpty()) {
        amount = parseAmount(amountStr);
        qDebug() << "  Parsed amount:" << amount;
      }
    } else {
      QString debitStr = getField(fields, idx.debit);
      QString creditStr = getField(fields, idx.credit);
      qDebug() << "  Debit:" << debitStr << "Credit:" << creditStr;
      if (!debitStr.isEmpty()) {
        amount = parseAmount(debitStr);
      } else if (!creditStr.isEmpty()) {
        amount = parseAmount(creditStr);
      }
      qDebug() << "  Parsed amount:" << amount;
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

    // Parse category (optional - prefer sub-category if available)
    QString category = getField(fields, idx.subCategory);
    if (category.isEmpty()) {
      category = getField(fields, idx.category);
    }

    // Create operation
    Operation *operation = new Operation(account);
    operation->set_date(date);
    operation->set_amount(amount);
    operation->set_description(description);
    operation->set_category(category);
    totalBalance += amount;

    account->addOperation(operation);
    importCount++;

    // Track the most recent imported operation
    if (!lastImportedOperation || operation->date() > lastImportedOperation->date()) {
      lastImportedOperation = operation;
    }
  }

  file.close();

  qDebug() << "Import complete:";
  qDebug() << "  Imported:" << importCount << "operations";
  qDebug() << "  Skipped:" << skippedCount << "rows";
  qDebug() << "  Total balance delta:" << totalBalance;

  // Note: We preserve the account balance from the YAML file.
  // The balance represents the current balance after all operations.

  // Set as current account
  int accountIndex = _accounts.indexOf(account);
  if (accountIndex >= 0) {
    set_currentAccountIndex(accountIndex);
  }

  emit accountCountChanged();
  emit operationCountChanged();

  // Select the last imported operation (most recent one)
  if (lastImportedOperation) {
    for (int i = 0; i < account->operationCount(); i++) {
      if (account->getOperation(i) == lastImportedOperation) {
        selectOperation(i, false);
        break;
      }
    }
  }

  emit dataLoaded();

  return true;
}

// Multi-selection implementation

bool BudgetData::isOperationSelected(int index) const {
  return _selectedOperations.contains(index);
}

void BudgetData::selectOperation(int index, bool extend) {
  if (index < 0 || index >= operationCount()) {
    return;
  }

  bool changed = false;

  if (!extend) {
    // Single selection: clear all and select just this one
    if (_selectedOperations.size() != 1 || !_selectedOperations.contains(index)) {
      _selectedOperations.clear();
      _selectedOperations.insert(index);
      changed = true;
    }
  } else {
    // Extend selection: add to existing selection
    if (!_selectedOperations.contains(index)) {
      _selectedOperations.insert(index);
      changed = true;
    }
  }

  _lastClickedIndex = index;

  // Also update the single selectedOperationIndex for compatibility
  set_selectedOperationIndex(index);

  if (changed) {
    emit selectedOperationsChanged();
  }
}

void BudgetData::toggleOperationSelection(int index) {
  if (index < 0 || index >= operationCount()) {
    return;
  }

  if (_selectedOperations.contains(index)) {
    _selectedOperations.remove(index);
  } else {
    _selectedOperations.insert(index);
  }

  _lastClickedIndex = index;

  // Update selectedOperationIndex to the toggled item
  set_selectedOperationIndex(index);

  emit selectedOperationsChanged();
}

void BudgetData::selectRange(int fromIndex, int toIndex) {
  if (fromIndex < 0) {
    fromIndex = 0;
  }
  if (toIndex < 0) {
    toIndex = 0;
  }

  int maxIndex = operationCount() - 1;
  if (fromIndex > maxIndex) {
    fromIndex = maxIndex;
  }
  if (toIndex > maxIndex) {
    toIndex = maxIndex;
  }

  if (fromIndex < 0 || toIndex < 0) {
    return; // No operations
  }

  int start = qMin(fromIndex, toIndex);
  int end = qMax(fromIndex, toIndex);

  // Add all indices in range to selection
  for (int i = start; i <= end; ++i) {
    _selectedOperations.insert(i);
  }

  _lastClickedIndex = toIndex;
  set_selectedOperationIndex(toIndex);

  emit selectedOperationsChanged();
}

void BudgetData::clearSelection() {
  if (!_selectedOperations.isEmpty()) {
    _selectedOperations.clear();
    _lastClickedIndex = -1;
    emit selectedOperationsChanged();
  }
}

int BudgetData::selectionCount() const {
  return _selectedOperations.size();
}

double BudgetData::selectedOperationsTotal() const {
  double total = 0.0;
  Account *account = currentAccount();
  if (!account) {
    return total;
  }

  for (int index : _selectedOperations) {
    Operation *op = account->getOperation(index);
    if (op) {
      total += op->amount();
    }
  }
  return total;
}

void BudgetData::copySelectedOperationsToClipboard() const {
  Account *account = currentAccount();
  if (!account || _selectedOperations.isEmpty()) {
    return;
  }

  // Build CSV content with header
  QString csv;
  csv += "Date,Description,Category,Amount,Balance\n";

  // Sort indices for consistent output
  QList<int> sortedIndices = _selectedOperations.values();
  std::sort(sortedIndices.begin(), sortedIndices.end());

  for (int index : sortedIndices) {
    Operation *op = account->getOperation(index);
    if (op) {
      // Escape fields that might contain commas or quotes
      QString description = op->description();
      QString category = op->category();

      // Escape quotes by doubling them
      description.replace("\"", "\"\"");
      category.replace("\"", "\"\"");

      // Quote fields that contain commas, quotes, or newlines
      if (description.contains(',') || description.contains('"') || description.contains('\n')) {
        description = "\"" + description + "\"";
      }
      if (category.contains(',') || category.contains('"') || category.contains('\n')) {
        category = "\"" + category + "\"";
      }

      // Format amount and balance with period as decimal separator (standard CSV format)
      QString amount = QString::number(op->amount(), 'f', 2);
      QString balance = QString::number(balanceAtIndex(index), 'f', 2);

      csv += op->date().toString("dd/MM/yyyy") + ","
           + description + ","
           + category + ","
           + amount + ","
           + balance + "\n";
    }
  }

  // Copy to clipboard
  QGuiApplication::clipboard()->setText(csv);
}
