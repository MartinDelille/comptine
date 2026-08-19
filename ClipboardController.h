#pragma once

#include <QObject>
#include <QQmlEngine>

class Account;

class ClipboardController2 : public QObject {
  Q_OBJECT
  QML_ELEMENT

public:
  explicit ClipboardController2(Account& account);

  // Copy selected operations to system clipboard as CSV
  Q_INVOKABLE void copySelectedOperations() const;

private:
  Account& _account;
};
