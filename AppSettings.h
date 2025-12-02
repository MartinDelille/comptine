#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include "PropertyMacros.h"

class AppSettings : public QObject {
  Q_OBJECT

  // Language: empty string = system default, "en" = English, "fr" = French
  PROPERTY_RW_CUSTOM(QString, language, QString())

public:
  explicit AppSettings(QObject *parent = nullptr);

signals:
  void languageChangeRequested();
};
