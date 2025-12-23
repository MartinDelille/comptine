#pragma once

#include <QtQml/qqml.h>
#include <QObject>
#include <QString>

#include "Category.h"
#include "PropertyMacros.h"

class Operation;

class CategorizationRule : public QObject {
  Q_OBJECT
  QML_ELEMENT

  PROPERTY_RW(const Category*, category, nullptr)
  PROPERTY_RW(QString, labelPrefix, QString())

public:
  explicit CategorizationRule(QObject* parent = nullptr);
  CategorizationRule(const Category* category,
                     const QString& labelPrefix,
                     QObject* parent = nullptr);

  // Check if this rule matches an operation
  Q_INVOKABLE bool matches(Operation* operation) const;

  // Check if this rule matches a label string (case-insensitive)
  Q_INVOKABLE bool matchesLabel(const QString& label) const;
};
