#include "Account.h"

Account::Account(QObject *parent) : QObject(parent) {}

Account::Account(const QString &name, double balance, QObject *parent)
    : QObject(parent), m_name(name), m_balance(balance) {}

QString Account::name() const {
  return m_name;
}

double Account::balance() const {
  return m_balance;
}

int Account::operationCount() const {
  return m_operations.size();
}

QList<Operation *> Account::operations() const {
  return m_operations;
}

void Account::setName(const QString &name) {
  if (m_name != name) {
    m_name = name;
    emit nameChanged();
  }
}

void Account::setBalance(double balance) {
  if (m_balance != balance) {
    m_balance = balance;
    emit balanceChanged();
  }
}

void Account::addOperation(Operation *operation) {
  if (operation) {
    operation->setParent(this);
    m_operations.append(operation);
    emit operationsChanged();
  }
}

void Account::removeOperation(int index) {
  if (index >= 0 && index < m_operations.size()) {
    delete m_operations.takeAt(index);
    emit operationsChanged();
  }
}

void Account::clearOperations() {
  qDeleteAll(m_operations);
  m_operations.clear();
  emit operationsChanged();
}

Operation *Account::getOperation(int index) const {
  if (index >= 0 && index < m_operations.size()) {
    return m_operations[index];
  }
  return nullptr;
}
