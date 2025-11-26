#pragma once

#include <QDate>
#include <QList>
#include <QObject>
#include <QRangeModel>
#include <QString>

class Transaction : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString accountingDate READ accountingDate CONSTANT)
  Q_PROPERTY(QString simplifiedLabel READ simplifiedLabel CONSTANT)
  Q_PROPERTY(QString operationLabel READ operationLabel CONSTANT)
  Q_PROPERTY(QString reference READ reference CONSTANT)
  Q_PROPERTY(QString additionalInfo READ additionalInfo CONSTANT)
  Q_PROPERTY(QString operationType READ operationType CONSTANT)
  Q_PROPERTY(QString category READ category CONSTANT)
  Q_PROPERTY(QString subCategory READ subCategory CONSTANT)
  Q_PROPERTY(QString debit READ debit CONSTANT)
  Q_PROPERTY(QString credit READ credit CONSTANT)
  Q_PROPERTY(QString operationDate READ operationDate CONSTANT)
  Q_PROPERTY(QString valueDate READ valueDate CONSTANT)
  Q_PROPERTY(QString checkStatus READ checkStatus CONSTANT)

public:
  explicit Transaction(QObject *parent = nullptr) : QObject(parent) {}

  QString accountingDate() const {
    return m_accountingDate.toString("dd/MM/yyyy");
  }
  QString simplifiedLabel() const { return m_simplifiedLabel; }
  QString operationLabel() const { return m_operationLabel; }
  QString reference() const { return m_reference; }
  QString additionalInfo() const { return m_additionalInfo; }
  QString operationType() const { return m_operationType; }
  QString category() const { return m_category; }
  QString subCategory() const { return m_subCategory; }
  QString debit() const {
    return m_debitValue == 0.0 ? QString()
                                : QString::number(m_debitValue, 'f', 2);
  }
  QString credit() const {
    return m_creditValue == 0.0 ? QString()
                                 : QString::number(m_creditValue, 'f', 2);
  }
  QString operationDate() const {
    return m_operationDate.toString("dd/MM/yyyy");
  }
  QString valueDate() const { return m_valueDate.toString("dd/MM/yyyy"); }
  QString checkStatus() const { return QString::number(m_checkStatus); }

  // Setters for loading data
  void setAccountingDate(const QDate &date) { m_accountingDate = date; }
  void setSimplifiedLabel(const QString &label) { m_simplifiedLabel = label; }
  void setOperationLabel(const QString &label) { m_operationLabel = label; }
  void setReference(const QString &ref) { m_reference = ref; }
  void setAdditionalInfo(const QString &info) { m_additionalInfo = info; }
  void setOperationType(const QString &type) { m_operationType = type; }
  void setCategory(const QString &cat) { m_category = cat; }
  void setSubCategory(const QString &subCat) { m_subCategory = subCat; }
  void setDebit(double value) { m_debitValue = value; }
  void setCredit(double value) { m_creditValue = value; }
  void setOperationDate(const QDate &date) { m_operationDate = date; }
  void setValueDate(const QDate &date) { m_valueDate = date; }
  void setCheckStatus(int status) { m_checkStatus = status; }

  // Getters for raw values (for coloring logic)
  double debitValue() const { return m_debitValue; }
  double creditValue() const { return m_creditValue; }

private:
  QDate m_accountingDate;
  QString m_simplifiedLabel;
  QString m_operationLabel;
  QString m_reference;
  QString m_additionalInfo;
  QString m_operationType;
  QString m_category;
  QString m_subCategory;
  double m_debitValue = 0.0;
  double m_creditValue = 0.0;
  QDate m_operationDate;
  QDate m_valueDate;
  int m_checkStatus = 0;
};

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
