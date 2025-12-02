#include "AppSettings.h"

AppSettings::AppSettings(QObject *parent)
    : QObject(parent) {
  QSettings settings;
  _language = settings.value("language", QString()).toString();
}

QString AppSettings::language() const {
  return _language;
}

void AppSettings::set_language(QString value) {
  if (_language != value) {
    _language = value;
    QSettings settings;
    settings.setValue("language", value);
    emit languageChanged();
    emit languageChangeRequested();
  }
}
