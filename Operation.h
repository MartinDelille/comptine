#pragma once

#include <QtQml/qqml.h>
#include <QDate>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariantList>

#include "Category.h"
#include "PropertyMacros.h"

// Represents one allocation in a split operation
struct CategoryAllocation {
  const Category* category;
  double amount;

  bool operator==(const CategoryAllocation& other) const {
    // qFuzzyCompare doesn't work well with zero values, use threshold comparison
    constexpr double epsilon = 0.0001;
    return category == other.category && std::abs(amount - other.amount) < epsilon;
  }

  bool operator!=(const CategoryAllocation& other) const { return !(*this == other); }
};

class Operation : public QObject {
  Q_OBJECT
  QML_ELEMENT

  PROPERTY_RW(QDate, date, {})
  PROPERTY_RW(double, amount, 0.0)
  PROPERTY_RW(const Category*, category, {})
  PROPERTY_RW(QString, description, {})

  // Budget date: returns date if not explicitly set
  PROPERTY_RW_CUSTOM(QDate, budgetDate, {})

  // Split allocations support
  Q_PROPERTY(QVariantList allocations READ allocations NOTIFY allocationsChanged)
  Q_PROPERTY(bool isSplit READ isSplit NOTIFY allocationsChanged)
  Q_PROPERTY(QString categoryDisplay READ categoryDisplay NOTIFY allocationsChanged)

public:
  explicit Operation(QObject* parent = nullptr);
  Operation(const QDate& date,
            double amount,
            const Category* category,
            const QString& description,
            const QList<CategoryAllocation>& allocations,
            QObject* parent = nullptr);

  // Split allocations methods
  QVariantList allocations() const;
  QList<CategoryAllocation> allocationsList() const { return _allocations; }
  void setAllocations(const QList<CategoryAllocation>& allocations);
  void clearAllocations();
  bool isSplit() const;
  QString categoryDisplay() const;

  // Get amount allocated to a specific category (for budget calculations)
  double amountForCategory(const Category* category) const;

signals:
  void allocationsChanged();

private:
  QList<CategoryAllocation> _allocations;
};
