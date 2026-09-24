#pragma once

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QQmlEngine>
#include <QString>

#include "utils/PropertyMacros.h"

class QNetworkAccessManager;
class AppSettings;

class UpdateController : public QObject {
  Q_OBJECT

  // Current state
  PROPERTY_RW(bool, checking, false)
  PROPERTY_RW(bool, updateAvailable, false)
  PROPERTY_RW(QString, latestVersion, {})
  PROPERTY_RW(QString, releaseNotes, {})
  PROPERTY_RW(QString, errorMessage, {})
  PROPERTY_RW(double, downloadProgress, 0.0)
  PROPERTY_RW(bool, downloading, false)
  PROPERTY_RW(bool, updateReady, false)
  PROPERTY_RW(bool, installSupported, false)

public:
  explicit UpdateController(AppSettings& appSettings);

  // Check for updates from GitHub releases
  Q_INVOKABLE void checkForUpdates();

  // Download the verified update selected by the latest manifest.
  Q_INVOKABLE void downloadUpdate();

  // Cancel an in-progress download and remove its temporary file.
  Q_INVOKABLE void cancelDownload();

  // Install the verified update. On macOS this starts the updater helper.
  Q_INVOKABLE void installUpdate();

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
  void updateDownloadCompleted();
  void updateDownloadFailed(const QString& error);
  void updateInstallFailed(const QString& error);

private slots:
  void onNetworkReply(QNetworkReply* reply);

private:
  bool isVersionNewer(const QString& remote, const QString& local) const;
  QList<int> parseVersion(const QString& version) const;
  bool parseManifest(const QByteArray& data);
  bool verifySignature(const QByteArray& message, const QByteArray& signature) const;
  bool verifyDownloadedUpdate(const QString& path) const;
  void failDownload(const QString& error);

  AppSettings& _appSettings;
  QNetworkAccessManager _networkManager;
  QNetworkReply* _downloadReply = nullptr;
  QString _downloadPath;
  QString _downloadUrl;
  QByteArray _downloadHash;
  QByteArray _downloadSignature;

  // GitHub repository information
  static constexpr const char* GITHUB_OWNER = "MartinDelille";
  static constexpr const char* GITHUB_REPO = "Comptine";
};
