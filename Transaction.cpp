#include "Transaction.h"

Transaction::Transaction(QObject *parent) :
    QObject(parent) {}

QString Transaction::accountingDate() const {
  return m_accountingDate.toString("dd/MM/yyyy");
}

QString Transaction::simplifiedLabel() const { return m_simplifiedLabel; }

QString Transaction::operationLabel() const { return m_operationLabel; }

QString Transaction::reference() const { return m_reference; }

QString Transaction::additionalInfo() const { return m_additionalInfo; }

QString Transaction::operationType() const { return m_operationType; }

QString Transaction::category() const { return m_category; }

QString Transaction::subCategory() const { return m_subCategory; }

QString Transaction::debit() const {
  return m_debitValue == 0.0 ? QString()
                             : QString::number(m_debitValue, 'f', 2);
}

QString Transaction::credit() const {
  return m_creditValue == 0.0 ? QString()
                              : QString::number(m_creditValue, 'f', 2);
}

QString Transaction::operationDate() const {
  return m_operationDate.toString("dd/MM/yyyy");
}

QString Transaction::valueDate() const {
  return m_valueDate.toString("dd/MM/yyyy");
}

QString Transaction::checkStatus() const {
  return QString::number(m_checkStatus);
}

void Transaction::setAccountingDate(const QDate &date) {
  m_accountingDate = date;
}

void Transaction::setSimplifiedLabel(const QString &label) {
  m_simplifiedLabel = label;
}

void Transaction::setOperationLabel(const QString &label) {
  m_operationLabel = label;
}

void Transaction::setReference(const QString &ref) { m_reference = ref; }

void Transaction::setAdditionalInfo(const QString &info) {
  m_additionalInfo = info;
}

void Transaction::setOperationType(const QString &type) {
  m_operationType = type;
}

void Transaction::setCategory(const QString &cat) { m_category = cat; }

void Transaction::setSubCategory(const QString &subCat) {
  m_subCategory = subCat;
}

void Transaction::setDebit(double value) { m_debitValue = value; }

void Transaction::setCredit(double value) { m_creditValue = value; }

void Transaction::setOperationDate(const QDate &date) {
  m_operationDate = date;
}

void Transaction::setValueDate(const QDate &date) { m_valueDate = date; }

void Transaction::setCheckStatus(int status) { m_checkStatus = status; }

double Transaction::debitValue() const { return m_debitValue; }

double Transaction::creditValue() const { return m_creditValue; }
