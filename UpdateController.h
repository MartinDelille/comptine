#pragma once

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

#include "PropertyMacros.h"

class QNetworkAccessManager;
class QNetworkReply;
class AppSettings;

class UpdateController : public QObject {
  Q_OBJECT

  // Current state
  PROPERTY_RW(bool, checking, false)
  PROPERTY_RW(bool, updateAvailable, false)
  PROPERTY_RW(QString, latestVersion, {})
  PROPERTY_RW(QString, releaseNotes, {})
  PROPERTY_RW(QString, downloadUrl, {})
  PROPERTY_RW(QString, errorMessage, {})

public:
  explicit UpdateController(AppSettings& appSettings);

  // Check for updates from GitHub releases
  Q_INVOKABLE void checkForUpdates();

  // Open the download page in the default browser
  Q_INVOKABLE void openDownloadPage();

  // Check if enough time has passed since last check (for auto-check)
  Q_INVOKABLE bool shouldAutoCheck() const;

  // Mark that an update check was performed
  Q_INVOKABLE void markUpdateChecked();

  // Current app version
  Q_INVOKABLE QString currentVersion() const;

signals:
  void updateCheckCompleted();
  void updateCheckFailed(const QString& error);

private slots:
  void onNetworkReply(QNetworkReply* reply);

private:
  bool isVersionNewer(const QString& remote, const QString& local) const;
  QList<int> parseVersion(const QString& version) const;

  AppSettings& _appSettings;
  QNetworkAccessManager _networkManager;

  // GitHub repository information
  static constexpr const char* GITHUB_OWNER = "MartinDelille";
  static constexpr const char* GITHUB_REPO = "Comptine";
};
