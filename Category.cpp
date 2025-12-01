#include "Category.h"

Category::Category(QObject *parent) : QObject(parent) {}

Category::Category(const QString &name, double budgetLimit, QObject *parent)
    : QObject(parent), m_name(name), m_budgetLimit(budgetLimit) {}

QString Category::name() const {
  return m_name;
}

double Category::budgetLimit() const {
  return m_budgetLimit;
}

void Category::setName(const QString &name) {
  if (m_name != name) {
    m_name = name;
    emit nameChanged();
  }
}

void Category::setBudgetLimit(double limit) {
  if (m_budgetLimit != limit) {
    m_budgetLimit = limit;
    emit budgetLimitChanged();
  }
}
