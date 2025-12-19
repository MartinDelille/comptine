#pragma once

#include <QtQml/qqml.h>
#include <QObject>
#include <QString>

#include "PropertyMacros.h"

class Operation;

class CategorizationRule : public QObject {
  Q_OBJECT
  QML_ELEMENT

  PROPERTY_RW(QString, category, QString())
  PROPERTY_RW(QString, descriptionPrefix, QString())

public:
  explicit CategorizationRule(QObject* parent = nullptr);
  CategorizationRule(const QString& category, const QString& descriptionPrefix,
                     QObject* parent = nullptr);

  // Check if this rule matches an operation
  Q_INVOKABLE bool matches(Operation* operation) const;

  // Check if this rule matches a description string (case-insensitive)
  Q_INVOKABLE bool matchesDescription(const QString& description) const;
};
