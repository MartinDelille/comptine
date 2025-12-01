#pragma once

#include <QObject>
#include <QString>

class Category : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString name READ name NOTIFY nameChanged)
  Q_PROPERTY(double budgetLimit READ budgetLimit NOTIFY budgetLimitChanged)

public:
  explicit Category(QObject *parent = nullptr);
  Category(const QString &name, double budgetLimit, QObject *parent = nullptr);

  QString name() const;
  double budgetLimit() const;

  void setName(const QString &name);
  void setBudgetLimit(double limit);

signals:
  void nameChanged();
  void budgetLimitChanged();

private:
  QString _name;
  double _budgetLimit = 0.0;
};
