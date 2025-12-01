#include "Account.h"

Account::Account(QObject *parent) : QObject(parent) {}

Account::Account(const QString &name, double balance, QObject *parent)
    : QObject(parent), _name(name), _balance(balance) {}

QString Account::name() const {
  return _name;
}

double Account::balance() const {
  return _balance;
}

int Account::operationCount() const {
  return _operations.size();
}

QList<Operation *> Account::operations() const {
  return _operations;
}

void Account::setName(const QString &name) {
  if (_name != name) {
    _name = name;
    emit nameChanged();
  }
}

void Account::setBalance(double balance) {
  if (_balance != balance) {
    _balance = balance;
    emit balanceChanged();
  }
}

void Account::addOperation(Operation *operation) {
  if (operation) {
    operation->setParent(this);
    _operations.append(operation);
    emit operationsChanged();
  }
}

void Account::removeOperation(int index) {
  if (index >= 0 && index < _operations.size()) {
    delete _operations.takeAt(index);
    emit operationsChanged();
  }
}

void Account::clearOperations() {
  qDeleteAll(_operations);
  _operations.clear();
  emit operationsChanged();
}

Operation *Account::getOperation(int index) const {
  if (index >= 0 && index < _operations.size()) {
    return _operations[index];
  }
  return nullptr;
}
