#pragma once

#include <QtQml/qqml.h>
#include <QAbstractListModel>
#include <QDate>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariantList>

#include "Category.h"
#include "utils/PropertyMacros.h"

class Account;

// Represents one allocation in a split operation
class Allocation : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(const Category* category READ category WRITE set_category NOTIFY categoryChanged)
  PROPERTY_RW(double, amount, 0.0)

public:
  explicit Allocation(const Category* c = nullptr, double a = 0.0, QObject* parent = nullptr) :
      QObject(parent), _amount(a), _category(c) {
  }

  const Category* category() const { return _category; }
  Q_INVOKABLE void set_category(const Category* value) {
    if (_category != value) {
      _category = value;
      emit categoryChanged();
    }
  }

  bool operator==(const Allocation& other) const {
    // qFuzzyCompare doesn't work well with zero values, use threshold comparison
    constexpr double epsilon = 0.0001;
    return _category == other._category && std::abs(_amount - other._amount) < epsilon;
  }

  bool operator!=(const Allocation& other) const { return !(*this == other); }

signals:
  void categoryChanged();

private:
  QPointer<const Category> _category;
};

class Operation : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Operations are created by the backend")

  Q_PROPERTY(int allocationCount READ rowCount NOTIFY allocationsChanged)
  PROPERTY_CONSTANT(Account*, account, nullptr)
  PROPERTY_RW(QDate, date, {})
  PROPERTY_RW(double, amount, 0.0)
  PROPERTY_RW(QString, label, {})
  PROPERTY_RW(QString, details, {})

  // Budget date: returns date if not explicitly set
  PROPERTY_RW_CUSTOM(QDate, budgetDate, {})

  // Split allocations support
  Q_PROPERTY(double allocatedAmount READ allocatedAmount NOTIFY allocationsChanged)
  Q_PROPERTY(bool isCategorized READ isCategorized NOTIFY allocationsChanged)
  Q_PROPERTY(QString categoryDisplay READ categoryDisplay NOTIFY allocationsChanged)
  Q_PROPERTY(QStringList allocatedCategoryNames READ allocatedCategoryNames NOTIFY allocationsChanged)

public:
  enum Roles {
    CategoryRole = Qt::UserRole + 1,
    AmountRole,
  };
  Q_ENUM(Roles)

  Operation(Account* account = nullptr,
            const QDate& date = {},
            double amount = 0.0,
            const QString& label = {},
            const QList<Allocation*>& allocations = {},
            const QString& details = {});
  ~Operation();

  // Split allocations methods
  QList<Allocation*> allocations() const { return _allocations; }
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;
  QStringList allocatedCategoryNames() const;
  void setAllocations(const QList<Allocation*>& allocations);
  void clearAllocations();
  bool sameAllocations(const QList<Allocation*>& otherAllocations) const;
  bool isCategorized() const;
  double allocatedAmount() const;
  QString categoryDisplay() const;

  // Get amount allocated to a specific category (for budget calculations)
  Q_INVOKABLE double amountForCategory(const Category* category) const;

signals:
  void allocationsChanged();

private:
  void connectCategorySignals(const QList<Allocation*>& allocations);
  void disconnectCategorySignals(const QList<Allocation*>& allocations);

  QList<Allocation*> _allocations;
};
