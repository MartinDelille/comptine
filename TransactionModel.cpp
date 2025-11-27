#include "TransactionModel.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>

TransactionModel::TransactionModel(QObject *parent)
    : QObject(parent),
      m_model(new QRangeModel(std::ref(m_labels), this)) {}

TransactionModel::~TransactionModel() {
  clearTransactions();
}

void TransactionModel::clearTransactions() {
  // Delete all Transaction objects
  qDeleteAll(m_transactions);
  m_transactions.clear();
}

bool TransactionModel::loadFromCsv(const QString &filePath) {
  qDebug() << "Loading CSV from:" << filePath;

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "Failed to open file:" << file.errorString();
    return false;
  }

  QList<Transaction*> newTransactions;

  QTextStream in(&file);
  // Set encoding to Latin1 (ISO-8859-1) for French bank CSV files
  in.setEncoding(QStringConverter::Latin1);

  // Skip header line
  if (!in.atEnd()) {
    QString header = in.readLine();
    qDebug() << "Header:" << header;
  }

  int lineCount = 0;
  while (!in.atEnd()) {
    QString line = in.readLine();
    lineCount++;
    QStringList fields = line.split(';');

    if (lineCount == 1) {
      qDebug() << "First line fields count:" << fields.size();
      qDebug() << "First line:" << line;
    }

    if (fields.size() >= 13) {
      // Create Transaction on the heap with 'this' as parent so it gets cleaned up
      Transaction *transaction = new Transaction(this);
      transaction->setAccountingDate(
          QDate::fromString(fields[0].trimmed(), "dd/MM/yyyy"));
      transaction->setSimplifiedLabel(fields[1].trimmed());
      transaction->setOperationLabel(fields[2].trimmed());
      transaction->setReference(fields[3].trimmed());
      transaction->setAdditionalInfo(fields[4].trimmed());
      transaction->setOperationType(fields[5].trimmed());
      transaction->setCategory(fields[6].trimmed());
      transaction->setSubCategory(fields[7].trimmed());

      // Parse debit (already negative in CSV)
      QString debitStr = fields[8].trimmed().replace(',', '.');
      if (!debitStr.isEmpty()) {
        transaction->setDebit(debitStr.toDouble());
      } else {
        transaction->setDebit(0.0);
      }

      // Parse credit (positive values with +)
      QString creditStr = fields[9].trimmed();
      if (!creditStr.isEmpty()) {
        creditStr = creditStr.replace('+', "").replace(',', '.');
        transaction->setCredit(creditStr.toDouble());
      } else {
        transaction->setCredit(0.0);
      }

      transaction->setOperationDate(
          QDate::fromString(fields[10].trimmed(), "dd/MM/yyyy"));
      transaction->setValueDate(
          QDate::fromString(fields[11].trimmed(), "dd/MM/yyyy"));
      transaction->setCheckStatus(fields[12].trimmed().toInt());

      newTransactions.append(transaction);
    }
  }

  file.close();
  qDebug() << "Loaded" << newTransactions.size() << "transactions";

  // Replace the data and recreate the model
  delete m_model;
  clearTransactions();
  m_transactions = std::move(newTransactions);
  
  // Extract simplified labels for the QRangeModel
  m_labels.clear();
  for (const Transaction* transaction : m_transactions) {
    m_labels.append(transaction->simplifiedLabel());
  }
  
  m_model = new QRangeModel(std::ref(m_labels), this);
  emit countChanged();
  emit modelChanged();

  return true;
}

void TransactionModel::clear() {
  delete m_model;
  clearTransactions();
  m_labels.clear();
  m_model = new QRangeModel(std::ref(m_labels), this);
  emit countChanged();
  emit modelChanged();
}

Transaction* TransactionModel::getTransaction(int index) const {
  if (index >= 0 && index < m_transactions.size()) {
    return m_transactions[index];
  }
  return nullptr;
}

double TransactionModel::balanceAtIndex(int index) const {
  if (index < 0 || index >= m_transactions.size()) {
    return 0.0;
  }
  // Transactions are sorted from most recent to oldest
  // Balance at index i = sum of amounts from i to end (oldest transactions first)
  double balance = 0.0;
  for (int i = m_transactions.size() - 1; i >= index; --i) {
    balance += m_transactions[i]->amount();
  }
  return balance;
}
