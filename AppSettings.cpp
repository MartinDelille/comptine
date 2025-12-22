#include <QFileInfo>

#include "AppSettings.h"

AppSettings::AppSettings() :
    _recentFilesModel(this) {
  _windowX = _settings.value("window/x", 200).toInt();
  _windowY = _settings.value("window/y", 200).toInt();
  _windowWidth = _settings.value("window/width", 1200).toInt();
  _windowHeight = _settings.value("window/height", 800).toInt();
  _language = _settings.value("language", QString()).toString();
  _theme = _settings.value("theme", QString()).toString();
  _checkForUpdates = _settings.value("checkForUpdates", true).toBool();
  _lastUpdateCheck = _settings.value("lastUpdateCheck", QDateTime()).toDateTime();
  _recentFilesModel.setStringList(
      _settings.value("recentFiles", QStringList()).toStringList());
}

int AppSettings::windowX() const {
  return _windowX;
}

void AppSettings::set_windowX(int value) {
  if (_windowX != value) {
    _windowX = value;
    _settings.setValue("window/x", value);
    _settings.sync();
    emit windowXChanged();
  }
}

int AppSettings::windowY() const {
  return _windowY;
}

void AppSettings::set_windowY(int value) {
  if (_windowY != value) {
    _windowY = value;
    _settings.setValue("window/y", value);
    _settings.sync();
    emit windowYChanged();
  }
}

int AppSettings::windowWidth() const {
  return _windowWidth;
}

void AppSettings::set_windowWidth(int value) {
  if (_windowWidth != value) {
    _windowWidth = value;
    _settings.setValue("window/width", value);
    _settings.sync();
    emit windowWidthChanged();
  }
}

int AppSettings::windowHeight() const {
  return _windowHeight;
}

void AppSettings::set_windowHeight(int value) {
  if (_windowHeight != value) {
    _windowHeight = value;
    _settings.setValue("window/height", value);
    _settings.sync();
    emit windowHeightChanged();
  }
}

QString AppSettings::language() const {
  return _language;
}

void AppSettings::set_language(QString value) {
  if (_language != value) {
    _language = value;
    _settings.setValue("language", value);
    _settings.sync();
    emit languageChanged();
    emit languageChangeRequested();
  }
}

QString AppSettings::theme() const {
  return _theme;
}

void AppSettings::set_theme(QString value) {
  if (_theme != value) {
    _theme = value;
    _settings.setValue("theme", value);
    _settings.sync();
    emit themeChanged();
  }
}

bool AppSettings::checkForUpdates() const {
  return _checkForUpdates;
}

void AppSettings::set_checkForUpdates(bool value) {
  if (_checkForUpdates != value) {
    _checkForUpdates = value;
    _settings.setValue("checkForUpdates", value);
    _settings.sync();
    emit checkForUpdatesChanged();
  }
}

QDateTime AppSettings::lastUpdateCheck() const {
  return _lastUpdateCheck;
}

void AppSettings::set_lastUpdateCheck(QDateTime value) {
  if (_lastUpdateCheck != value) {
    _lastUpdateCheck = value;
    _settings.setValue("lastUpdateCheck", value);
    _settings.sync();
    emit lastUpdateCheckChanged();
  }
}

QStringListModel* AppSettings::recentFilesModel() {
  return &_recentFilesModel;
}

QStringList AppSettings::recentFiles() const {
  return _recentFilesModel.stringList();
}

void AppSettings::addRecentFile(const QString& filePath) {
  // Find if file already exists in the list
  QStringList files = _recentFilesModel.stringList();
  int existingIndex = files.indexOf(filePath);

  if (existingIndex == 0) {
    // Already at the front, nothing to do
    return;
  }

  if (existingIndex > 0) {
    // Remove from current position
    _recentFilesModel.removeRow(existingIndex);
  }

  // Insert at front
  _recentFilesModel.insertRow(0);
  _recentFilesModel.setData(_recentFilesModel.index(0), filePath);

  // Remove excess items from the end
  while (_recentFilesModel.rowCount() > MaxRecentFiles) {
    _recentFilesModel.removeRow(_recentFilesModel.rowCount() - 1);
  }

  saveRecentFiles();
}

void AppSettings::clearRecentFiles() {
  if (_recentFilesModel.rowCount() > 0) {
    _recentFilesModel.removeRows(0, _recentFilesModel.rowCount());
    saveRecentFiles();
  }
}

void AppSettings::saveRecentFiles() {
  _settings.setValue("recentFiles", _recentFilesModel.stringList());
  _settings.sync();
}
