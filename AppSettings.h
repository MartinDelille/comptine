#pragma once

#include <QDateTime>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringListModel>

#include "PropertyMacros.h"

class AppSettings : public QObject {
  Q_OBJECT

  PROPERTY_RW_CUSTOM(int, windowX, 200)
  PROPERTY_RW_CUSTOM(int, windowY, 200)
  PROPERTY_RW_CUSTOM(int, windowWidth, 1200)
  PROPERTY_RW_CUSTOM(int, windowHeight, 800)

  // Language: empty string = system default, "en" = English, "fr" = French
  PROPERTY_RW_CUSTOM(QString, language, QString())

  // Theme: empty string = system default, "light" = Light, "dark" = Dark
  PROPERTY_RW_CUSTOM(QString, theme, QString())

  // Auto-check for updates on startup
  PROPERTY_RW_CUSTOM(bool, checkForUpdates, true)

  // Last time we checked for updates
  PROPERTY_RW_CUSTOM(QDateTime, lastUpdateCheck, QDateTime())

  // Recent files model for proper QML binding
  Q_PROPERTY(QStringListModel* recentFilesModel READ recentFilesModel CONSTANT)

public:
  explicit AppSettings();

  QStringListModel* recentFilesModel();
  QStringList recentFiles() const;
  Q_INVOKABLE void addRecentFile(const QString& filePath);
  Q_INVOKABLE void clearRecentFiles();

  static constexpr int MaxRecentFiles = 10;

signals:
  void languageChangeRequested();

private:
  void saveRecentFiles();

  QSettings _settings;
  QStringListModel _recentFilesModel;
};
