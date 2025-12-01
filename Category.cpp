#include "Category.h"

Category::Category(QObject *parent) : QObject(parent) {}

Category::Category(const QString &name, double budgetLimit, QObject *parent)
    : QObject(parent), _name(name), _budgetLimit(budgetLimit) {}

QString Category::name() const {
  return _name;
}

double Category::budgetLimit() const {
  return _budgetLimit;
}

void Category::setName(const QString &name) {
  if (_name != name) {
    _name = name;
    emit nameChanged();
  }
}

void Category::setBudgetLimit(double limit) {
  if (_budgetLimit != limit) {
    _budgetLimit = limit;
    emit budgetLimitChanged();
  }
}
