#include "Operation.h"

Operation::Operation(QObject *parent) : QObject(parent) {}

Operation::Operation(const QDate &date, double amount, const QString &category,
                     const QString &description, QObject *parent)
    : QObject(parent), _date(date), _amount(amount), _category(category),
      _description(description) {}

QString Operation::date() const {
  return _date.toString("yyyy-MM-dd");
}

QDate Operation::dateValue() const {
  return _date;
}

double Operation::amount() const {
  return _amount;
}

QString Operation::category() const {
  return _category;
}

QString Operation::description() const {
  return _description;
}

void Operation::setDate(const QDate &date) {
  if (_date != date) {
    _date = date;
    emit dateChanged();
  }
}

void Operation::setAmount(double amount) {
  if (_amount != amount) {
    _amount = amount;
    emit amountChanged();
  }
}

void Operation::setCategory(const QString &category) {
  if (_category != category) {
    _category = category;
    emit categoryChanged();
  }
}

void Operation::setDescription(const QString &description) {
  if (_description != description) {
    _description = description;
    emit descriptionChanged();
  }
}
