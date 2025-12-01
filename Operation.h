#pragma once

#include <QDate>
#include <QObject>
#include <QString>

class Operation : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString date READ date NOTIFY dateChanged)
  Q_PROPERTY(double amount READ amount NOTIFY amountChanged)
  Q_PROPERTY(QString category READ category NOTIFY categoryChanged)
  Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)

public:
  explicit Operation(QObject *parent = nullptr);
  Operation(const QDate &date, double amount, const QString &category,
            const QString &description, QObject *parent = nullptr);

  QString date() const;
  QDate dateValue() const;
  double amount() const;
  QString category() const;
  QString description() const;

  void setDate(const QDate &date);
  void setAmount(double amount);
  void setCategory(const QString &category);
  void setDescription(const QString &description);

signals:
  void dateChanged();
  void amountChanged();
  void categoryChanged();
  void descriptionChanged();

private:
  QDate _date;
  double _amount = 0.0;
  QString _category;
  QString _description;
};
