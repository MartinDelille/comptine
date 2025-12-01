#include "BudgetData.h"
#include <QDate>
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

BudgetData::BudgetData(QObject *parent)
    : QObject(parent) {}

BudgetData::~BudgetData() {
  clear();
}

QString BudgetData::currentFilePath() const {
  return m_currentFilePath;
}

int BudgetData::accountCount() const {
  return m_accounts.size();
}

QList<Account *> BudgetData::accounts() const {
  return m_accounts;
}

Account *BudgetData::getAccount(int index) const {
  if (index >= 0 && index < m_accounts.size()) {
    return m_accounts[index];
  }
  return nullptr;
}

Account *BudgetData::getAccountByName(const QString &name) const {
  for (Account *account : m_accounts) {
    if (account->name() == name) {
      return account;
    }
  }
  return nullptr;
}

void BudgetData::addAccount(Account *account) {
  if (account) {
    account->setParent(this);
    m_accounts.append(account);
    if (m_currentAccountIndex < 0) {
      setCurrentAccountIndex(0);
    }
    emit accountsChanged();
  }
}

void BudgetData::removeAccount(int index) {
  if (index >= 0 && index < m_accounts.size()) {
    delete m_accounts.takeAt(index);
    if (m_currentAccountIndex >= m_accounts.size()) {
      setCurrentAccountIndex(m_accounts.size() - 1);
    }
    emit accountsChanged();
  }
}

void BudgetData::clearAccounts() {
  qDeleteAll(m_accounts);
  m_accounts.clear();
  m_currentAccountIndex = -1;
  emit accountsChanged();
  emit currentAccountChanged();
}

Account *BudgetData::currentAccount() const {
  return getAccount(m_currentAccountIndex);
}

void BudgetData::setCurrentAccountIndex(int index) {
  if (index != m_currentAccountIndex && index >= -1 && index < m_accounts.size()) {
    m_currentAccountIndex = index;
    emit currentAccountChanged();
    emit operationsChanged();
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

  // Start with account balance and subtract operations from 0 to index-1
  // Operations are sorted from most recent to oldest
  double balance = account->balance();
  for (int i = account->operationCount() - 1; i >= index; --i) {
    Operation *op = account->getOperation(i);
    if (op) {
      balance += op->amount();
    }
  }
  return balance;
}

int BudgetData::categoryCount() const {
  return m_categories.size();
}

QList<Category *> BudgetData::categories() const {
  return m_categories;
}

Category *BudgetData::getCategory(int index) const {
  if (index >= 0 && index < m_categories.size()) {
    return m_categories[index];
  }
  return nullptr;
}

Category *BudgetData::getCategoryByName(const QString &name) const {
  for (Category *category : m_categories) {
    if (category->name() == name) {
      return category;
    }
  }
  return nullptr;
}

void BudgetData::addCategory(Category *category) {
  if (category) {
    category->setParent(this);
    m_categories.append(category);
    emit categoriesChanged();
  }
}

void BudgetData::removeCategory(int index) {
  if (index >= 0 && index < m_categories.size()) {
    delete m_categories.takeAt(index);
    emit categoriesChanged();
  }
}

void BudgetData::clearCategories() {
  qDeleteAll(m_categories);
  m_categories.clear();
  emit categoriesChanged();
}

void BudgetData::clear() {
  clearAccounts();
  clearCategories();
}

QString BudgetData::escapeYamlString(const QString &str) const {
  if (str.contains('\n') || str.contains('"') || str.contains(':') ||
      str.contains('#') || str.startsWith(' ') || str.endsWith(' ')) {
    QString escaped = str;
    escaped.replace("\\", "\\\\");
    escaped.replace("\"", "\\\"");
    escaped.replace("\n", "\\n");
    return "\"" + escaped + "\"";
  }
  return str;
}

QString BudgetData::unescapeYamlString(const QString &str) const {
  QString result = str.trimmed();
  if (result.startsWith('"') && result.endsWith('"')) {
    result = result.mid(1, result.length() - 2);
    result.replace("\\n", "\n");
    result.replace("\\\"", "\"");
    result.replace("\\\\", "\\");
  }
  return result;
}

bool BudgetData::saveToYaml(const QString &filePath) const {
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    qWarning() << "Failed to open file for writing:" << filePath;
    return false;
  }

  QTextStream out(&file);
  out.setEncoding(QStringConverter::Utf8);

  // Write categories
  out << "categories:\n";
  for (const Category *category : m_categories) {
    out << "  - name: " << escapeYamlString(category->name()) << "\n";
    out << "    budget_limit: " << category->budgetLimit() << "\n";
  }

  // Write accounts
  out << "\naccounts:\n";
  for (const Account *account : m_accounts) {
    out << "  - name: " << escapeYamlString(account->name()) << "\n";
    out << "    balance: " << account->balance() << "\n";
    out << "    operations:\n";
    for (const Operation *op : account->operations()) {
      out << "      - date: " << op->date() << "\n";
      out << "        amount: " << op->amount() << "\n";
      out << "        category: " << escapeYamlString(op->category()) << "\n";
      out << "        description: " << escapeYamlString(op->description())
          << "\n";
    }
  }

  file.close();
  qDebug() << "Budget data saved to:" << filePath;
  emit const_cast<BudgetData*>(this)->dataSaved();
  return true;
}

bool BudgetData::loadFromYaml(const QString &filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "Failed to open file for reading:" << filePath;
    return false;
  }

  clear();

  QTextStream in(&file);
  in.setEncoding(QStringConverter::Utf8);

  enum class Section { None, Categories, Accounts, Operations };
  Section currentSection = Section::None;

  Account *currentAccount = nullptr;
  Category *currentCategory = nullptr;
  Operation *currentOperation = nullptr;

  // Match "key: value" or "- key: value" patterns
  QRegularExpression keyValueRe("^(\\s*)-?\\s*([\\w_]+):\\s*(.*)$");

  while (!in.atEnd()) {
    QString line = in.readLine();

    // Skip empty lines and comments
    if (line.trimmed().isEmpty() || line.trimmed().startsWith('#')) {
      continue;
    }

    auto match = keyValueRe.match(line);
    if (!match.hasMatch()) {
      continue;
    }

    int indent = match.captured(1).length();
    QString key = match.captured(2);
    QString value = unescapeYamlString(match.captured(3));

    // List items (indicated by "- key: value" pattern)
    bool isListItem = line.trimmed().startsWith('-');

    // Top-level sections
    if (indent == 0) {
      if (key == "categories") {
        currentSection = Section::Categories;
      } else if (key == "accounts") {
        currentSection = Section::Accounts;
      }
      continue;
    }

    if (currentSection == Section::Categories) {
      if (isListItem && key == "name") {
        currentCategory = new Category(this);
        currentCategory->setName(value);
        m_categories.append(currentCategory);
      } else if (currentCategory && key == "budget_limit") {
        currentCategory->setBudgetLimit(value.toDouble());
      }
    } else if (currentSection == Section::Accounts) {
      if (indent == 2 && isListItem && key == "name") {
        currentAccount = new Account(this);
        currentAccount->setName(value);
        m_accounts.append(currentAccount);
        currentOperation = nullptr;
      } else if (currentAccount && indent == 4 && key == "balance") {
        currentAccount->setBalance(value.toDouble());
      } else if (currentAccount && indent == 4 && key == "operations") {
        currentSection = Section::Operations;
      }
    } else if (currentSection == Section::Operations && currentAccount) {
      if (indent == 6 && isListItem && key == "date") {
        currentOperation = new Operation(currentAccount);
        currentOperation->setDate(QDate::fromString(value, "yyyy-MM-dd"));
        currentAccount->addOperation(currentOperation);
      } else if (currentOperation) {
        if (key == "amount") {
          currentOperation->setAmount(value.toDouble());
        } else if (key == "category") {
          currentOperation->setCategory(value);
        } else if (key == "description") {
          currentOperation->setDescription(value);
        }
      }

      // Check if we're back to accounts section
      if (indent == 2 && isListItem) {
        currentSection = Section::Accounts;
        currentAccount = new Account(this);
        currentAccount->setName(value);
        m_accounts.append(currentAccount);
        currentOperation = nullptr;
      }
    }
  }

  file.close();

  // Set first account as current
  if (!m_accounts.isEmpty()) {
    setCurrentAccountIndex(0);
  }

  m_currentFilePath = filePath;
  emit filePathChanged();
  emit accountsChanged();
  emit categoriesChanged();
  emit dataLoaded();
  qDebug() << "Budget data loaded from:" << filePath;
  qDebug() << "  Accounts:" << m_accounts.size();
  qDebug() << "  Categories:" << m_categories.size();

  return true;
}

bool BudgetData::importFromCsv(const QString &filePath, const QString &accountName) {
  qDebug() << "Importing CSV from:" << filePath;

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "Failed to open file:" << file.errorString();
    return false;
  }

  // Create or get account
  QString name = accountName.isEmpty() ? "Imported Account" : accountName;
  Account *account = getAccountByName(name);
  if (!account) {
    account = new Account(name, 0.0, this);
    m_accounts.append(account);
  }

  QTextStream in(&file);
  // Set encoding to Latin1 (ISO-8859-1) for French bank CSV files
  in.setEncoding(QStringConverter::Latin1);

  // Skip header line
  if (!in.atEnd()) {
    QString header = in.readLine();
    qDebug() << "Header:" << header;
  }

  int importCount = 0;
  double totalBalance = 0.0;

  while (!in.atEnd()) {
    QString line = in.readLine();
    QStringList fields = line.split(';');

    if (fields.size() >= 13) {
      Operation *operation = new Operation(account);

      // Parse date (field 0: accounting date)
      operation->setDate(QDate::fromString(fields[0].trimmed(), "dd/MM/yyyy"));

      // Parse amount (field 8: debit, field 9: credit)
      QString debitStr = fields[8].trimmed().replace(',', '.');
      QString creditStr = fields[9].trimmed().replace('+', "").replace(',', '.');

      double amount = 0.0;
      if (!debitStr.isEmpty()) {
        amount = debitStr.toDouble(); // Already negative
      } else if (!creditStr.isEmpty()) {
        amount = creditStr.toDouble();
      }
      operation->setAmount(amount);
      totalBalance += amount;

      // Parse category (field 6: category, field 7: sub-category)
      QString category = fields[6].trimmed();
      QString subCategory = fields[7].trimmed();
      if (!subCategory.isEmpty()) {
        category = subCategory; // Use sub-category as more specific
      }
      operation->setCategory(category);

      // Parse description (field 1: simplified label)
      operation->setDescription(fields[1].trimmed());

      account->addOperation(operation);
      importCount++;

      // Auto-create category if it doesn't exist
      if (!category.isEmpty() && !getCategoryByName(category)) {
        Category *cat = new Category(category, 0.0, this);
        m_categories.append(cat);
      }
    }
  }

  file.close();

  // Update account balance
  account->setBalance(totalBalance);

  // Set as current account
  int accountIndex = m_accounts.indexOf(account);
  if (accountIndex >= 0) {
    setCurrentAccountIndex(accountIndex);
  }

  emit accountsChanged();
  emit categoriesChanged();
  emit dataLoaded();

  qDebug() << "Imported" << importCount << "operations";
  qDebug() << "Total balance:" << totalBalance;

  return true;
}
