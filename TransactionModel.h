#pragma once

#include <QAbstractTableModel>
#include <QDateTime>
#include <QString>
#include <QVector>

struct Transaction {
  QDate accountingDate;
  QString simplifiedLabel;
  QString operationLabel;
  QString reference;
  QString additionalInfo;
  QString operationType;
  QString category;
  QString subCategory;
  double debit;
  double credit;
  QDate operationDate;
  QDate valueDate;
  int checkStatus;
};

class TransactionModel : public QAbstractTableModel {
  Q_OBJECT
  Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
  enum TransactionRoles {
    AccountingDateRole = Qt::UserRole + 1,
    SimplifiedLabelRole,
    OperationLabelRole,
    ReferenceRole,
    AdditionalInfoRole,
    OperationTypeRole,
    CategoryRole,
    SubCategoryRole,
    DebitRole,
    CreditRole,
    OperationDateRole,
    ValueDateRole,
    CheckStatusRole
  };

  explicit TransactionModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE bool loadFromCsv(const QString &filePath);
  Q_INVOKABLE void clear();
  int count() const { return m_transactions.size(); }

signals:
  void countChanged();

private:
  QVector<Transaction> m_transactions;
};
