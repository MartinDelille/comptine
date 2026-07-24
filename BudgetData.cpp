#include <QClipboard>
#include <QDate>
#include <QDebug>
#include <QGuiApplication>

#include "Account.h"
#include "BudgetData.h"
#include "NavigationController.h"
#include "UndoCommands.h"

BudgetData::BudgetData(QUndoStack& undoStack) :
    _undoStack(undoStack) {
}

BudgetData::~BudgetData() {
  clear();
}

int BudgetData::currentAccountIndex() const {
  return accountIndex(_currentAccount);
}

void BudgetData::set_currentAccountIndex(int index) {
  set_currentAccount(accountAt(index));
}

int BudgetData::rowCount(const QModelIndex& parent) const {
  if (parent.isValid())
    return 0;

  return _accounts.size();
}

QVariant BudgetData::data(const QModelIndex& index, int role) const {
  if (!index.isValid())
    return QVariant();

  Account* account = accountAt(index.row());
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

Account* BudgetData::accountAt(int index) const {
  if (index >= 0 && index < _accounts.size()) {
    return _accounts[index];
  }
  return nullptr;
}

Account* BudgetData::accountByName(const QString& name) const {
  for (Account* account : _accounts) {
    if (account->name() == name) {
      return account;
    }
  }
  return nullptr;
}

QString BudgetData::suggestedAccountForFile(const QString& filename) const {
  for (Account* account : _accounts) {
    for (QString source : account->importSourcePrefixes()) {
      if (filename.startsWith(source)) {
        return account->name();
      }
    }
  }
  return {};
}

int BudgetData::accountIndex(Account* account) const {
  return _accounts.indexOf(account);
}

void BudgetData::renameCurrentAccount(const QString& newName) {
  Account* account = currentAccount();
  if (account && !newName.isEmpty() && account->name() != newName) {
    _undoStack.push(new RenameAccountCommand(*account,
                                             account->name(), newName));
  }
}

void BudgetData::addAccount(Account* account) {
  if (account) {
    connect(account, &Account::nameChanged, this, [this, account] {
      auto index = createIndex(this->accountIndex(account), 0);
      emit dataChanged(index, index);
    });
    beginInsertRows(QModelIndex(), _accounts.size(), _accounts.size());
    account->setParent(this);
    _accounts.append(account);
    endInsertRows();
    emit accountCountChanged();
  }
}

void BudgetData::removeAccount(int index) {
  if (index >= 0 && index < _accounts.size()) {
    delete _accounts.takeAt(index);
    emit accountCountChanged();
  }
}

Account* BudgetData::takeAccount(Account* account) {
  int index = _accounts.indexOf(account);
  if (index >= 0) {
    // If this is the current account, update navigation before removing
    if (account == currentAccount()) {
      // Select previous account, or -1 if this was the only account
      set_currentAccount(account);
    } else if (currentAccountIndex() > index) {
      // Adjust index if removing an account before the current one
      set_currentAccountIndex(currentAccountIndex() - 1);
    }

    Account* acc = _accounts.takeAt(index);
    acc->setParent(nullptr);  // Release Qt ownership
    emit accountCountChanged();
    return acc;
  }
  return nullptr;
}

void BudgetData::clearAccounts() {
  beginResetModel();
  qDeleteAll(_accounts);
  _accounts.clear();
  endResetModel();
  emit accountCountChanged();
}

void BudgetData::addOperation(const QDate& date, double amount, const QString& label, const QString& details, const QList<Allocation*>& allocations) {
  Account* account = currentAccount();
  if (!account) return;

  auto operation = new Operation(account, date, amount, label, details, allocations);
  _undoStack.push(new AddOperationCommand(operation, *account));
}

void BudgetData::setOperationBudgetDate(Operation* operation, const QDate& newBudgetDate) {
  if (!operation) return;

  QDate oldBudgetDate = operation->budgetDate();
  if (oldBudgetDate != newBudgetDate) {
    _undoStack.push(new SetOperationBudgetDateCommand(*operation,
                                                      oldBudgetDate, newBudgetDate));
  }
}

void BudgetData::setOperationAmount(Operation* operation, double newAmount) {
  if (!operation) return;

  double oldAmount = operation->amount();
  if (!qFuzzyCompare(oldAmount, newAmount)) {
    _undoStack.push(new SetOperationAmountCommand(*operation,
                                                  oldAmount, newAmount));
  }
}

void BudgetData::setOperationDate(Operation* operation, const QDate& newDate) {
  if (!operation) return;

  QDate oldDate = operation->date();
  if (oldDate != newDate) {
    _undoStack.push(new SetOperationDateCommand(*operation,
                                                oldDate, newDate));
  }
}

void BudgetData::setOperationLabel(Operation* operation, const QString& newLabel) {
  if (!operation) return;

  QString oldLabel = operation->label();
  if (oldLabel != newLabel) {
    _undoStack.push(new SetOperationLabelCommand(*operation,
                                                 oldLabel, newLabel));
  }
}

void BudgetData::setOperationDetails(Operation* operation, const QString& newDetails) {
  if (!operation) return;

  QString oldDetails = operation->details();
  if (oldDetails != newDetails) {
    _undoStack.push(new SetOperationDetailsCommand(*operation,
                                                   oldDetails, newDetails));
  }
}

void BudgetData::setOperationAllocations(Operation* operation, const QList<Allocation*>& allocations) {
  if (!operation) return;

  // Only create command if something changed
  if (!operation->sameAllocations(allocations)) {
    _undoStack.push(new SplitOperationCommand(*operation,
                                              allocations));
  } else {
    qDeleteAll(allocations);
  }
}

Operation* BudgetData::createCounterPart(Operation* operation, Account* targetAccount, const QString& categoryName) {
  if (!operation || !targetAccount) return nullptr;

  QList<Allocation*> newAllocations;
  double amount = 0;
  for (auto allocation : operation->allocations()) {
    if (categoryName.isEmpty() || (allocation->category() && allocation->category()->name() == categoryName)) {
      amount -= allocation->amount();
      newAllocations.append(new Allocation(allocation->category(), -allocation->amount()));
    }
  }

  auto newOperation = new Operation(targetAccount, operation->date(), amount,
                                    operation->label(), operation->details(), newAllocations);

  _undoStack.push(new AddOperationCommand(newOperation, *targetAccount));
  return newOperation;
}

void BudgetData::deleteSelectedOperations() {
  Account* account = currentAccount();
  if (!account) return;

  QUndoCommand* macroCommand = new QUndoCommand();

  for (Operation* op : account->selectedOperations()) {
    new DeleteOperationCommand(op, *account, macroCommand);
  }
  _undoStack.push(macroCommand);
}

void BudgetData::clear() {
  clearAccounts();
  _undoStack.clear();
  _undoStack.setClean();
}

void BudgetData::copySelectedOperations() const {
  Account* account = currentAccount();
  if (!account) return;

  QString csv = account->selectedOperationsAsCsv();
  if (!csv.isEmpty()) {
    QGuiApplication::clipboard()->setText(csv);
  }
}
