#pragma once

#include "Operation.h"
#include <QList>
#include <QObject>
#include <QString>

class Account : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString name READ name NOTIFY nameChanged)
  Q_PROPERTY(double balance READ balance NOTIFY balanceChanged)
  Q_PROPERTY(int operationCount READ operationCount NOTIFY operationsChanged)

public:
  explicit Account(QObject *parent = nullptr);
  Account(const QString &name, double balance, QObject *parent = nullptr);

  QString name() const;
  double balance() const;
  int operationCount() const;
  QList<Operation *> operations() const;

  void setName(const QString &name);
  void setBalance(double balance);
  void addOperation(Operation *operation);
  void removeOperation(int index);
  void clearOperations();

  Q_INVOKABLE Operation *getOperation(int index) const;

signals:
  void nameChanged();
  void balanceChanged();
  void operationsChanged();

private:
  QString _name;
  double _balance = 0.0;
  QList<Operation *> _operations;
};
