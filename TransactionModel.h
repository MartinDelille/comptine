#pragma once

#include "Transaction.h"
#include <QList>
#include <QObject>
#include <QRangeModel>
#include <QString>

class TransactionModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(int count READ count NOTIFY countChanged)
  Q_PROPERTY(QAbstractItemModel *model READ model NOTIFY modelChanged)

public:
  explicit TransactionModel(QObject *parent = nullptr);
  ~TransactionModel();

  Q_INVOKABLE bool loadFromCsv(const QString &filePath);
  Q_INVOKABLE void clear();
  int count() const { return m_labels.size(); }
  QAbstractItemModel *model() const { return m_model; }

signals:
  void countChanged();
  void modelChanged();

private:
  void clearTransactions();
  
  // Keep full transaction data for future use
  QList<Transaction*> m_transactions;
  
  // Simple list for QRangeModel to display
  QList<QString> m_labels;
  QRangeModel *m_model;
};
