#pragma once

#include <QtQml/qqml.h>
#include <QAbstractListModel>

#include "Account.h"

class AccountListModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
  enum Roles {
    NameRole = Qt::UserRole + 1,
    OperationCountRole,
    AccountRole
  };
  Q_ENUM(Roles)

  explicit AccountListModel(QList<Account*>& accounts, QObject* parent = nullptr);

  // QAbstractListModel interface
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  // Refresh the model view
  void refresh();

signals:
  void countChanged();

private:
  QList<Account*>& _accounts;
};
