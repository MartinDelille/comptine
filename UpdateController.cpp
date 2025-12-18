#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include "AppSettings.h"
#include "UpdateController.h"
#include "Version.h"

UpdateController::UpdateController(AppSettings& appSettings) :
    _appSettings(appSettings) {
  connect(&_networkManager, &QNetworkAccessManager::finished,
          this, &UpdateController::onNetworkReply);
}

void UpdateController::checkForUpdates() {
  if (_checking) {
    return;
  }

  set_checking(true);
  set_errorMessage({});
  set_updateAvailable(false);

  // Build GitHub API URL for latest release
  QString apiUrl = QString("https://api.github.com/repos/%1/%2/releases/latest")
                       .arg(GITHUB_OWNER)
                       .arg(GITHUB_REPO);

  QNetworkRequest request(apiUrl);
  request.setHeader(QNetworkRequest::UserAgentHeader, "Comptine-UpdateChecker");
  request.setRawHeader("Accept", "application/vnd.github.v3+json");

  _networkManager.get(request);
}

void UpdateController::onNetworkReply(QNetworkReply* reply) {
  reply->deleteLater();
  set_checking(false);

  if (reply->error() != QNetworkReply::NoError) {
    QString error = reply->errorString();
    set_errorMessage(error);
    emit updateCheckFailed(error);
    return;
  }

  QByteArray data = reply->readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);

  if (!doc.isObject()) {
    QString error = tr("Invalid response from GitHub");
    set_errorMessage(error);
    emit updateCheckFailed(error);
    return;
  }

  QJsonObject release = doc.object();

  // Extract version from tag_name
  QString version = release["tag_name"].toString();

  set_latestVersion(version);

  // Extract release notes from body
  QString body = release["body"].toString();
  set_releaseNotes(body);

  // Get the HTML URL for the release page
  QString htmlUrl = release["html_url"].toString();
  set_downloadUrl(htmlUrl);

  // Check if this version is newer
  QString current = currentVersion();
  bool isNewer = isVersionNewer(version, current);
  set_updateAvailable(isNewer);

  emit updateCheckCompleted();
}

void UpdateController::openDownloadPage() {
  if (!_downloadUrl.isEmpty()) {
    QDesktopServices::openUrl(QUrl(_downloadUrl));
  }
}

bool UpdateController::shouldAutoCheck() const {
  // Check if auto-update is enabled
  if (!_appSettings.checkForUpdates()) {
    return false;
  }

  // Check if enough time has passed (1 day minimum between checks)
  QDateTime lastCheck = _appSettings.lastUpdateCheck();
  if (!lastCheck.isValid()) {
    return true;  // Never checked before
  }

  qint64 secondsSinceLastCheck = lastCheck.secsTo(QDateTime::currentDateTime());
  constexpr qint64 ONE_DAY_IN_SECONDS = 24 * 60 * 60;

  return secondsSinceLastCheck >= ONE_DAY_IN_SECONDS;
}

void UpdateController::markUpdateChecked() {
  _appSettings.set_lastUpdateCheck(QDateTime::currentDateTime());
}

QString UpdateController::currentVersion() const {
  return APP_VERSION;
}

bool UpdateController::isVersionNewer(const QString& remote, const QString& local) const {
  QList<int> remoteParts = parseVersion(remote);
  QList<int> localParts = parseVersion(local);

  // Compare each part
  int maxParts = qMax(remoteParts.size(), localParts.size());
  for (int i = 0; i < maxParts; ++i) {
    int remotePart = (i < remoteParts.size()) ? remoteParts[i] : 0;
    int localPart = (i < localParts.size()) ? localParts[i] : 0;

    if (remotePart > localPart) {
      return true;
    } else if (remotePart < localPart) {
      return false;
    }
  }

  return false;  // Versions are equal
}

QList<int> UpdateController::parseVersion(const QString& version) const {
  QList<int> parts;

  // Remove suffix after hyphen (e.g., "-dev-abc123")
  QString cleanVersion = version;
  int hyphenIndex = cleanVersion.indexOf('-');
  if (hyphenIndex > 0) {
    cleanVersion = cleanVersion.left(hyphenIndex);
  }

  // Split by dots and parse as integers
  QStringList stringParts = cleanVersion.split('.');
  for (const QString& part : stringParts) {
    bool ok;
    int value = part.toInt(&ok);
    if (ok) {
      parts.append(value);
    }
  }

  return parts;
}
