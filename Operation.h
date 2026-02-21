#pragma once

#include <QtQml/qqml.h>
#include <QDate>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariantList>

#include "Category.h"
#include "PropertyMacros.h"

class Account;

// Represents one allocation in a split operation
class Allocation : public QObject {
  Q_OBJECT
  QML_ELEMENT

  PROPERTY_RW(const Category*, category, nullptr)
  PROPERTY_RW(double, amount, 0.0)

public:
  explicit Allocation(const Category* c = nullptr, double a = 0.0, QObject* parent = nullptr) :
      QObject(parent), _category(c), _amount(a) {
  }

  bool operator==(const Allocation& other) const {
    // qFuzzyCompare doesn't work well with zero values, use threshold comparison
    constexpr double epsilon = 0.0001;
    return _category == other._category && std::abs(_amount - other._amount) < epsilon;
  }

  bool operator!=(const Allocation& other) const { return !(*this == other); }
};

class Operation : public QObject {
  Q_OBJECT
  QML_ELEMENT

  PROPERTY_CONSTANT(Account*, account, nullptr)
  PROPERTY_RW(QDate, date, {})
  PROPERTY_RW(double, amount, 0.0)
  PROPERTY_RW(QString, label, {})
  PROPERTY_RW(QString, details, {})

  // Budget date: returns date if not explicitly set
  PROPERTY_RW_CUSTOM(QDate, budgetDate, {})

  // Split allocations support
  Q_PROPERTY(QList<Allocation*> allocations READ allocations NOTIFY allocationsChanged)
  Q_PROPERTY(bool isCategorized READ isCategorized NOTIFY allocationsChanged)
  Q_PROPERTY(QString categoryDisplay READ categoryDisplay NOTIFY allocationsChanged)

public:
  Operation(Account* account = nullptr,
            const QDate& date = {},
            double amount = 0.0,
            const QString& label = {},
            const QString& details = {},
            const QList<Allocation*>& allocations = {});

  // Split allocations methods
  QList<Allocation*> allocations() const { return _allocations; }
  void setAllocations(const QList<Allocation*>& allocations);
  void clearAllocations();
  bool sameAllocations(const QList<Allocation*>& otherAllocations) const;
  bool isCategorized() const;
  QString categoryDisplay() const;

  // Get amount allocated to a specific category (for budget calculations)
  double amountForCategory(const Category* category) const;

signals:
  void allocationsChanged();

private:
  QList<Allocation*> _allocations;
};
