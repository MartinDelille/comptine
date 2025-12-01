#include "Operation.h"

Operation::Operation(QObject *parent) : QObject(parent) {}

Operation::Operation(const QDate &date, double amount, const QString &category,
                     const QString &description, QObject *parent)
    : QObject(parent), m_date(date), m_amount(amount), m_category(category),
      m_description(description) {}

QString Operation::date() const {
  return m_date.toString("yyyy-MM-dd");
}

QDate Operation::dateValue() const {
  return m_date;
}

double Operation::amount() const {
  return m_amount;
}

QString Operation::category() const {
  return m_category;
}

QString Operation::description() const {
  return m_description;
}

void Operation::setDate(const QDate &date) {
  if (m_date != date) {
    m_date = date;
    emit dateChanged();
  }
}

void Operation::setAmount(double amount) {
  if (m_amount != amount) {
    m_amount = amount;
    emit amountChanged();
  }
}

void Operation::setCategory(const QString &category) {
  if (m_category != category) {
    m_category = category;
    emit categoryChanged();
  }
}

void Operation::setDescription(const QString &description) {
  if (m_description != description) {
    m_description = description;
    emit descriptionChanged();
  }
}
