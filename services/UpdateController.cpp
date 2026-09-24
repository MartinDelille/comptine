#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>
#include <QSysInfo>
#include <QUrl>

#include <openssl/evp.h>

#include "AppSettings.h"
#include "UpdateController.h"
#include "Version.h"

using namespace Qt::StringLiterals;

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
  set_updateReady(false);
  set_downloadProgress(0.0);

  QString apiUrl = u"https://github.com/%1/%2/releases/latest/download/Comptine-update.json"_s
                       .arg(GITHUB_OWNER)
                       .arg(GITHUB_REPO);
#ifdef COMPTINE_ENABLE_UPDATE_TEST_OVERRIDE
  const QString testUrl = qEnvironmentVariable("COMPTINE_UPDATE_MANIFEST_URL");
  if (!testUrl.isEmpty())
    apiUrl = testUrl;
#endif
  qInfo() << "Checking for updates at" << apiUrl;

  QNetworkRequest request(apiUrl);
  request.setHeader(QNetworkRequest::UserAgentHeader, "Comptine-UpdateChecker");
  request.setTransferTimeout(15000);
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                       QNetworkRequest::NoLessSafeRedirectPolicy);

  QNetworkReply* reply = _networkManager.get(request);
  connect(reply, &QNetworkReply::downloadProgress, this,
          [this](qint64 received, qint64 total) {
            if (total > 0)
              set_downloadProgress(static_cast<double>(received) / static_cast<double>(total));
          });
}

void UpdateController::onNetworkReply(QNetworkReply* reply) {
  if (reply == _downloadReply)
    return;
  reply->deleteLater();
  set_checking(false);

  qInfo() << "Update manifest response from" << reply->url()
          << "HTTP status" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

  if (reply->error() != QNetworkReply::NoError) {
    QString error = reply->errorString();
    qWarning() << "Update check failed:" << error;
    set_errorMessage(error);
    emit updateCheckFailed(error);
    return;
  }

  QByteArray data = reply->readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);

  if (!doc.isObject()) {
    QString error = tr("Invalid response from GitHub");
    qWarning() << "Update manifest is not a JSON object";
    set_errorMessage(error);
    emit updateCheckFailed(error);
    return;
  }

  if (!parseManifest(data)) {
    QString error = tr("Invalid or unsigned update manifest");
    qWarning() << "Update manifest was rejected";
    set_errorMessage(error);
    emit updateCheckFailed(error);
    return;
  }

  emit updateCheckCompleted();
}

void UpdateController::downloadUpdate() {
  if (_downloadReply || _downloadUrl.isEmpty() || !updateAvailable())
    return;

  QString directory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                      + "/updates";
  if (!QDir().mkpath(directory)) {
    failDownload(tr("Could not create the update directory"));
    return;
  }

  QString filename = QFileInfo(QUrl(_downloadUrl).path()).fileName();
  if (filename.isEmpty())
    filename = "Comptine-update.download";
  _downloadPath = directory + "/" + filename;
  qInfo() << "Downloading update from" << _downloadUrl << "to" << _downloadPath;
  QFile::remove(_downloadPath);
  set_errorMessage({});
  set_updateReady(false);
  set_downloadProgress(0.0);
  set_checking(false);

  QNetworkRequest request{ QUrl(_downloadUrl) };
  request.setHeader(QNetworkRequest::UserAgentHeader, "Comptine-Updater");
  request.setTransferTimeout(120000);
  _downloadReply = _networkManager.get(request);
  set_downloading(true);

  connect(_downloadReply, &QNetworkReply::downloadProgress, this,
          [this](qint64 received, qint64 total) {
            if (total > 0)
              set_downloadProgress(static_cast<double>(received) / static_cast<double>(total));
          });
  connect(_downloadReply, &QNetworkReply::finished, this, [this]() {
    QNetworkReply* reply = _downloadReply;
    _downloadReply = nullptr;
    set_downloading(false);
    if (reply->error() == QNetworkReply::OperationCanceledError) {
      qInfo() << "Update download canceled";
      QFile::remove(_downloadPath);
      set_downloadProgress(0.0);
      reply->deleteLater();
      return;
    }
    if (reply->error() != QNetworkReply::NoError || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() >= 400) {
      qWarning() << "Update download failed:" << reply->errorString()
                 << "HTTP status" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
      failDownload(reply->errorString().isEmpty() ? tr("Update download failed")
                                                  : reply->errorString());
      reply->deleteLater();
      return;
    }
    QSaveFile file(_downloadPath);
    if (!file.open(QIODevice::WriteOnly) || file.write(reply->readAll()) < 0 || !file.commit() || !verifyDownloadedUpdate(_downloadPath)) {
      qWarning() << "Downloaded update failed verification at" << _downloadPath;
      failDownload(tr("The downloaded update failed verification"));
      reply->deleteLater();
      return;
    }
    set_downloadProgress(1.0);
    set_updateReady(true);
    qInfo() << "Update downloaded and verified successfully:" << _downloadPath;
    emit updateDownloadCompleted();
    reply->deleteLater();
  });
}

void UpdateController::cancelDownload() {
  if (_downloadReply) {
    _downloadReply->abort();
    return;
  }
  if (!_downloadPath.isEmpty())
    QFile::remove(_downloadPath);
  set_downloading(false);
  set_downloadProgress(0.0);
}

void UpdateController::installUpdate() {
  if (!updateReady() || _downloadPath.isEmpty())
    return;
#ifdef Q_OS_MACOS
  QString helper = QCoreApplication::applicationDirPath() + "/../Helpers/ComptineUpdater";
  qInfo() << "Starting update helper" << helper << "with update" << _downloadPath;
  if (!QFileInfo::exists(helper)) {
    qWarning() << "Update helper does not exist:" << helper;
    emit updateInstallFailed(tr("The update helper is not installed"));
    return;
  }
  if (!QProcess::startDetached(helper, { QCoreApplication::applicationFilePath(), _downloadPath,
                                         QString::number(QCoreApplication::applicationPid()) })) {
    qWarning() << "Could not start update helper:" << helper;
    emit updateInstallFailed(tr("Could not start the update installer"));
    return;
  }
  QCoreApplication::exit(0);
#else
  if (_downloadPath.endsWith(".AppImage", Qt::CaseInsensitive)) {
    QFile::setPermissions(_downloadPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner | QFileDevice::ReadGroup | QFileDevice::ExeGroup | QFileDevice::ReadOther | QFileDevice::ExeOther);
  }
  QDesktopServices::openUrl(QUrl::fromLocalFile(_downloadPath));
#endif
}

bool UpdateController::parseManifest(const QByteArray& data) {
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (!doc.isObject())
    return false;
  QJsonObject root = doc.object();
  QString version = root["version"].toString();
  if (version.isEmpty()) {
    qWarning() << "Update manifest has no version";
    return false;
  }

  qInfo() << "Update manifest version:" << version << "current version:" << currentVersion();

  set_latestVersion(version);
  set_releaseNotes(root["releaseNotes"].toString());
  if (!isVersionNewer(version, currentVersion())) {
    qInfo() << "No newer update is available";
    set_updateAvailable(false);
    return true;
  }

  QString platform;
#ifdef Q_OS_MACOS
  platform = "macos";
#elif defined(Q_OS_WIN)
  platform = "windows";
#else
  platform = "linux";
#endif
  QString architecture = QSysInfo::currentCpuArchitecture();
  QJsonObject selected;
  for (const QJsonValue& value : root["assets"].toArray()) {
    QJsonObject asset = value.toObject();
    if (asset["platform"].toString() == platform && (asset["architecture"].toString() == architecture || asset["architecture"].toString() == "universal")) {
      selected = asset;
      break;
    }
  }
  QString url = selected["url"].toString();
  QByteArray hash = QByteArray::fromHex(selected["sha256"].toString().toLatin1());
  QByteArray signature = QByteArray::fromBase64(selected["signature"].toString().toLatin1());
  if (url.isEmpty() || !QUrl(url).isValid() || hash.size() != QCryptographicHash::hashLength(QCryptographicHash::Sha256) || signature.isEmpty())
    qWarning() << "Update manifest has no valid asset for platform" << platform
               << "architecture" << architecture;
  if (url.isEmpty() || !QUrl(url).isValid() || hash.size() != QCryptographicHash::hashLength(QCryptographicHash::Sha256) || signature.isEmpty())
    return false;

  QByteArray signedPayload = (version + "\n" + platform + "\n" + selected["architecture"].toString() + "\n" + url + "\n" + selected["sha256"].toString()).toUtf8();
  if (!verifySignature(signedPayload, signature)) {
    qWarning() << "Update manifest signature verification failed for" << url;
    return false;
  }

  qInfo() << "Selected verified update asset" << url << "for" << platform << architecture;

  _downloadUrl = url;
  _downloadHash = hash;
  _downloadSignature = signature;
  set_installSupported(platform == "macos");
  set_updateAvailable(true);
  return true;
}

bool UpdateController::verifySignature(const QByteArray& message,
                                       const QByteArray& signature) const {
  QByteArray publicKey = QByteArray::fromBase64(
      "QxX5HudOzF1dzVVtQf86d/F7qODAhtfapFeiM4hwyDw=");
#ifdef COMPTINE_ENABLE_UPDATE_TEST_OVERRIDE
  const QByteArray testKey = qgetenv("COMPTINE_UPDATE_PUBLIC_KEY");
  if (!testKey.isEmpty())
    publicKey = QByteArray::fromBase64(testKey);
#endif
  EVP_PKEY* key = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr,
                                              reinterpret_cast<const unsigned char*>(publicKey.constData()),
                                              publicKey.size());
  if (!key)
    return false;
  EVP_MD_CTX* context = EVP_MD_CTX_new();
  bool valid = context && EVP_DigestVerifyInit(context, nullptr, nullptr, nullptr, key) == 1 && EVP_DigestVerify(context, reinterpret_cast<const unsigned char*>(signature.constData()), signature.size(), reinterpret_cast<const unsigned char*>(message.constData()), message.size()) == 1;
  EVP_MD_CTX_free(context);
  EVP_PKEY_free(key);
  return valid;
}

bool UpdateController::verifyDownloadedUpdate(const QString& path) const {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return false;
  QCryptographicHash hash(QCryptographicHash::Sha256);
  while (!file.atEnd())
    hash.addData(file.read(qint64{ 1024 } * 1024));
  return hash.result() == _downloadHash;
}

void UpdateController::failDownload(const QString& error) {
  if (!_downloadPath.isEmpty())
    QFile::remove(_downloadPath);
  set_errorMessage(error);
  set_downloadProgress(0.0);
  emit updateDownloadFailed(error);
}

void UpdateController::openDownloadPage() {
  QDesktopServices::openUrl(QUrl("https://martin.delille.org/comptine"_L1));
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
  constexpr qint64 ONE_DAY_IN_SECONDS = qint64{ 24 } * 60 * 60;

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
  int maxParts = static_cast<int>(qMax(remoteParts.size(), localParts.size()));
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
  int hyphenIndex = static_cast<int>(cleanVersion.indexOf('-'));
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
