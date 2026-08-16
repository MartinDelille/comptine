#include <QClipboard>
#include <QDate>
#include <QDebug>
#include <QFileInfo>
#include <QGuiApplication>

#include "BudgetData.h"
#include "UndoCommands.h"
#include "model/Account.h"

BudgetData::BudgetData(QUndoStack& undoStack) :
    _budgetDate(QDate::currentDate()),
    _undoStack(undoStack) {
}

BudgetData::~BudgetData() {
  clear();
}

int BudgetData::currentAccountIndex() const {
  return accountIndex(_currentAccount);
}

void BudgetData::set_currentAccountIndex(int index) {
  set_currentAccount(at(index));
}

void BudgetData::previousMonth() {
  QDate date = _budgetDate.addMonths(-1);
  set_budgetDate(QDate(date.year(), date.month(), 1));
}

void BudgetData::nextMonth() {
  QDate date = _budgetDate.addMonths(1);
  set_budgetDate(QDate(date.year(), date.month(), 1));
}

void BudgetData::navigateToOperation(Operation* operation) {
  auto account = operation->account();
  // Switch to the account
  set_currentAccount(account);

  // Find the operation in the account
  account->select(operation, false);

  set_currentTabIndex(0);
}
int BudgetData::rowCount(const QModelIndex& parent) const {
  if (parent.isValid())
    return 0;

  return _accounts.size();
}

QVariant BudgetData::data(const QModelIndex& index, int role) const {
  if (!index.isValid())
    return QVariant();

  auto account = at(index.row());
  if (account == nullptr)
    return QVariant();

  switch (static_cast<Roles>(role)) {
    case NameRole:
      return account->name();
    case OperationCountRole:
      return account->rowCount();
    case AccountRole:
      return QVariant::fromValue(account);
  }

  return QVariant();
}

QHash<int, QByteArray> BudgetData::roleNames() const {
  return {
    { NameRole, "name" },
    { OperationCountRole, "operationCount" },
    { AccountRole, "account" }
  };
}

QList<Account*> BudgetData::accounts() const {
  return _accounts;
}

Account* BudgetData::at(int index) const {
  if (index >= 0 && index < _accounts.size()) {
    return _accounts[index];
  }
  return nullptr;
}

Account* BudgetData::accountByName(const QString& name) const {
  for (auto account : _accounts) {
    if (account->name() == name) {
      return account;
    }
  }
  return nullptr;
}

QString BudgetData::suggestedAccountForUrl(const QUrl& url) const {
  auto baseName = QFileInfo(url.toLocalFile()).baseName();
  for (auto account : _accounts) {
    if (account->name().compare(baseName, Qt::CaseInsensitive) == 0) {
      return account->name();
    }
    for (QString source : account->importSourcePrefixes()) {
      if (baseName.toLower().startsWith(source.toLower())) {
        return account->name();
      }
    }
  }
  return baseName;
}

int BudgetData::accountIndex(Account* account) const {
  return _accounts.indexOf(account);
}

Account* BudgetData::addAccount(Account* account) {
  if (account == nullptr) {
    return nullptr;
  }
  connect(account, &Account::nameChanged, this, [this, account] {
    auto index = createIndex(this->accountIndex(account), 0);
    emit dataChanged(index, index);
  });
  beginInsertRows(QModelIndex(), _accounts.size(), _accounts.size());
  account->setParent(this);
  _accounts.append(account);
  endInsertRows();
  emit accountCountChanged();
  connect(account, &Account::operationDataChanged,
          this, &BudgetData::operationDataChanged);
  emit operationDataChanged();
  return account;
}

Account* BudgetData::createAccount(const QString& name) {
  Q_ASSERT(accountByName(name) == nullptr);
  return addAccount(new Account(name, this));
}

void BudgetData::removeAccount(int index) {
  if (index >= 0 && index < _accounts.size()) {
    beginRemoveRows(QModelIndex(), index, index);
    delete _accounts.takeAt(index);
    endRemoveRows();
    emit accountCountChanged();
    emit operationDataChanged();
  }
}

Account* BudgetData::takeAccount(Account* account) {
  int index = _accounts.indexOf(account);
  if (index >= 0) {
    // If this is the current account, update navigation before removing
    if (account == _currentAccount) {
      // Select previous account, or -1 if this was the only account
      set_currentAccount(account);
    } else if (currentAccountIndex() > index) {
      // Adjust index if removing an account before the current one
      set_currentAccountIndex(currentAccountIndex() - 1);
    }

    auto acc = _accounts.takeAt(index);
    acc->setParent(nullptr);
    emit accountCountChanged();
    return acc;
  }
  return nullptr;
}

void BudgetData::clearAccounts() {
  if (_accounts.empty()) {
    return;
  }
  beginResetModel();
  qDeleteAll(_accounts);
  _accounts.clear();
  endResetModel();
  emit accountCountChanged();
  emit operationDataChanged();
}

int BudgetData::countOperationsWithCategory(const Category* category) const {
  int count = 0;
  for (auto account : _accounts) {
    count += account->countOperationsWithCategory(category);
  }
  return count;
}

void BudgetData::clear() {
  clearAccounts();
  _undoStack.clear();
  _undoStack.setClean();
}

void BudgetData::copySelectedOperations() const {
  auto account = currentAccount();
  if (!account) return;

  QString csv = account->selectedOperationsAsCsv();
  if (!csv.isEmpty()) {
    QGuiApplication::clipboard()->setText(csv);
  }
}
