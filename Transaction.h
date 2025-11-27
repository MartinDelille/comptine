#pragma once

#include <QDate>
#include <QObject>
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
  Q_PROPERTY(double debit READ debit CONSTANT)
  Q_PROPERTY(double credit READ credit CONSTANT)
  Q_PROPERTY(double amount READ amount CONSTANT)
  Q_PROPERTY(QString operationDate READ operationDate CONSTANT)
  Q_PROPERTY(QString valueDate READ valueDate CONSTANT)
  Q_PROPERTY(QString checkStatus READ checkStatus CONSTANT)

public:
  explicit Transaction(QObject *parent = nullptr);

  QString accountingDate() const;
  QString simplifiedLabel() const;
  QString operationLabel() const;
  QString reference() const;
  QString additionalInfo() const;
  QString operationType() const;
  QString category() const;
  QString subCategory() const;
  double debit() const;
  double credit() const;
  double amount() const;
  QString operationDate() const;
  QString valueDate() const;
  QString checkStatus() const;

  // Setters for loading data
  void setAccountingDate(const QDate &date);
  void setSimplifiedLabel(const QString &label);
  void setOperationLabel(const QString &label);
  void setReference(const QString &ref);
  void setAdditionalInfo(const QString &info);
  void setOperationType(const QString &type);
  void setCategory(const QString &cat);
  void setSubCategory(const QString &subCat);
  void setDebit(double value);
  void setCredit(double value);
  void setOperationDate(const QDate &date);
  void setValueDate(const QDate &date);
  void setCheckStatus(int status);

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
