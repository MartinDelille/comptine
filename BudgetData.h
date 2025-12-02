#pragma once

#include "Account.h"
#include "Category.h"
#include "PropertyMacros.h"
#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>

class BudgetData : public QObject {
  Q_OBJECT

  // UI state properties (macro-generated)
  PROPERTY_RW(int, selectedOperationIndex, 0)
  PROPERTY_RW(int, currentTabIndex, 0)
  PROPERTY_RW(int, budgetYear, 0)
  PROPERTY_RW(int, budgetMonth, 0)

  // Data properties (macro-generated)
  PROPERTY_RW(QString, currentFilePath, {})

  // Read-only computed properties (macro-generated, implemented in .cpp)
  PROPERTY_RO(int, accountCount)
  PROPERTY_RO(int, categoryCount)
  PROPERTY_RO(int, operationCount)
  PROPERTY_RO(Account *, currentAccount)

  // Custom property with validation logic (implemented in .cpp)
  PROPERTY_RW_CUSTOM(int, currentAccountIndex, -1)

public:
  explicit BudgetData(QObject *parent = nullptr);
  ~BudgetData();

  // Account management
  QList<Account *> accounts() const;
  Q_INVOKABLE Account *getAccount(int index) const;
  Q_INVOKABLE Account *getAccountByName(const QString &name) const;
  void addAccount(Account *account);
  void removeAccount(int index);
  void clearAccounts();

  // Operations from current account (for UI binding)
  Q_INVOKABLE Operation *getOperation(int index) const;
  Q_INVOKABLE double balanceAtIndex(int index) const;

  // Category management
  QList<Category *> categories() const;
  Q_INVOKABLE Category *getCategory(int index) const;
  Q_INVOKABLE Category *getCategoryByName(const QString &name) const;
  void addCategory(Category *category);
  void removeCategory(int index);
  void clearCategories();

  // Budget calculations (aggregates across all accounts)
  Q_INVOKABLE double spentInCategory(const QString &categoryName, int year, int month) const;
  Q_INVOKABLE QVariantList monthlyBudgetSummary(int year, int month) const;

  // File operations
  Q_INVOKABLE bool loadFromYaml(const QString &filePath);
  Q_INVOKABLE bool saveToYaml(const QString &filePath) const;
  Q_INVOKABLE bool importFromCsv(const QString &filePath, const QString &accountName = QString());

  // Clear all data
  Q_INVOKABLE void clear();

signals:
  void dataLoaded();
  void dataSaved();

private:
  QString escapeYamlString(const QString &str) const;
  QString unescapeYamlString(const QString &str) const;

  QList<Account *> _accounts;
  QList<Category *> _categories;
};
