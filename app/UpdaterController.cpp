#include "UpdaterController.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <signal.h>
#include <unistd.h>
#endif

namespace {

#ifdef Q_OS_MACOS
QString shellQuote(const QString& value) {
  QString quoted = value;
  quoted.replace("'", "'\\''");
  return "'" + quoted + "'";
}
#endif

}  // namespace

UpdaterController::UpdaterController(QString executable, QString packagePath, qint64 parentPid) :
    _executable(std::move(executable)),
    _packagePath(std::move(packagePath)),
    _parentPid(parentPid) {
  connect(&_parentTimer, &QTimer::timeout, this, &UpdaterController::checkParentProcess);
  connect(&_process, &QProcess::finished, this, &UpdaterController::processFinished);
  connect(&_process, &QProcess::errorOccurred, this, &UpdaterController::processError);

  QDir appDirectory(QFileInfo(_executable).absolutePath());
  appDirectory.cdUp();
  appDirectory.cdUp();
  _appBundle = appDirectory.absolutePath();
}

void UpdaterController::start() {
  if (_finished || _failed)
    return;

  qInfo() << "Updater started for executable" << _executable << "and package" << _packagePath
          << "waiting for pid" << _parentPid;

  if (_executable.isEmpty() || _packagePath.isEmpty() || _parentPid <= 0) {
    setError(tr("Invalid updater arguments"));
    return;
  }
  if (!QFileInfo::exists(_executable) || !QFileInfo::exists(_packagePath)) {
    setError(tr("The application or update package is missing"));
    return;
  }

  setStatus(tr("Waiting for Comptine to close…"), 0.0);
  if (processIsRunning()) {
    _parentTimer.start(250);
  } else {
    parentExited();
  }
}

void UpdaterController::checkParentProcess() {
  if (processIsRunning())
    return;

  _parentTimer.stop();
  parentExited();
}

bool UpdaterController::processIsRunning() const {
#ifdef Q_OS_WIN
  HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                               static_cast<DWORD>(_parentPid));
  if (!process)
    return false;
  DWORD exitCode = 0;
  const bool running = GetExitCodeProcess(process, &exitCode) && exitCode == STILL_ACTIVE;
  CloseHandle(process);
  return running;
#else
  return kill(static_cast<pid_t>(_parentPid), 0) == 0;
#endif
}

void UpdaterController::parentExited() {
  qInfo() << "Comptine exited; starting update operation";
#ifdef Q_OS_MACOS
  _mountPoint = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                    .filePath(QString("ComptineUpdate-%1").arg(QCoreApplication::applicationPid()));
  if (!QDir().mkpath(_mountPoint)) {
    setError(tr("Could not create the update mount directory"));
    return;
  }
  _replacement = QDir(_mountPoint).filePath("Comptine.app");
  setStatus(tr("Mounting update…"), 0.2);
  startProcess("/usr/bin/hdiutil", { "attach", _packagePath, "-nobrowse", "-readonly", "-mountpoint", _mountPoint }, Step::Mount);
#elif defined(Q_OS_WIN)
  setStatus(tr("Installing update…"), 0.2);
  startProcess(_packagePath, { "/S" }, Step::Install);
#else
  installLinuxUpdate();
#endif
}

void UpdaterController::startProcess(const QString& program, const QStringList& arguments, Step step) {
  _step = step;
  _process.start(program, arguments);
}

void UpdaterController::processError(QProcess::ProcessError error) {
  qWarning() << "Update process error" << error << _process.errorString();
  if (error == QProcess::FailedToStart)
    setError(tr("Could not start the update operation"));
}

void UpdaterController::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
  if (_failed || exitStatus != QProcess::NormalExit || exitCode != 0) {
#ifdef Q_OS_MACOS
    if (_step == Step::Copy && !_copyRetried) {
      _copyRetried = true;
      const QString command = QString("/usr/bin/ditto %1 %2")
                                  .arg(shellQuote(_replacement), shellQuote(_appBundle));
      startProcess("/usr/bin/osascript",
                   { "-e", QString("do shell script %1 with administrator privileges")
                               .arg(shellQuote(command)) },
                   Step::Copy);
      return;
    }
#endif
    setError(tr("The update operation failed"));
    return;
  }

#ifdef Q_OS_MACOS
  switch (_step) {
    case Step::Mount:
      if (!QFileInfo::exists(_replacement)) {
        setError(tr("The update image does not contain Comptine.app"));
        return;
      }
      setStatus(tr("Verifying update…"), 0.4);
      startProcess("/usr/bin/codesign", { "--verify", "--deep", "--strict", _replacement }, Step::Verify);
      return;
    case Step::Verify:
      setStatus(tr("Installing update…"), 0.6);
      startProcess("/usr/bin/ditto", { _replacement, _appBundle }, Step::Copy);
      return;
    case Step::Copy:
      setStatus(tr("Finishing update…"), 0.8);
      startProcess("/usr/bin/hdiutil", { "detach", _mountPoint, "-quiet" }, Step::Detach);
      return;
    case Step::Detach:
      finishSuccessfully();
      return;
    case Step::None:
    case Step::Install:
      break;
    default:
      break;
  }
#elif defined(Q_OS_WIN)
  if (_step == Step::Install) {
    finishSuccessfully();
    return;
  }
#endif

  setError(tr("The update operation completed unexpectedly"));
}

void UpdaterController::installLinuxUpdate() {
  setStatus(tr("Installing update…"), 0.5);
  _installTarget = qEnvironmentVariable("APPIMAGE");
  if (_installTarget.isEmpty() || !QFileInfo::exists(_installTarget))
    _installTarget = _executable;

  const QString backup = _installTarget + ".old";
  QFile::remove(backup);
  if (!QFile::rename(_installTarget, backup) || !QFile::copy(_packagePath, _installTarget)) {
    QFile::remove(_installTarget);
    QFile::rename(backup, _installTarget);
    setError(tr("Could not replace the application"));
    return;
  }
  QFile::remove(backup);
  QFile::setPermissions(_installTarget, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner | QFileDevice::ReadGroup | QFileDevice::ExeGroup | QFileDevice::ReadOther | QFileDevice::ExeOther);
  finishSuccessfully();
}

void UpdaterController::finishSuccessfully() {
  cleanup();
  setStatus(tr("Restarting Comptine…"), 1.0, false);
  _finished = true;
  emit finishedChanged();
#ifndef Q_OS_WIN
  relaunchApplication();
#endif
  QTimer::singleShot(500, QCoreApplication::instance(), &QCoreApplication::quit);
}

void UpdaterController::relaunchApplication() {
#ifdef Q_OS_WIN
  QProcess::startDetached(_executable);
#elif defined(Q_OS_MACOS)
  QProcess::startDetached("/usr/bin/open", { _appBundle });
#else
  QProcess::startDetached(_installTarget.isEmpty() ? _executable : _installTarget);
#endif
}

void UpdaterController::cleanup() {
#ifdef Q_OS_MACOS
  if (!_mountPoint.isEmpty())
    QDir().remove(_mountPoint);
#endif
  QFile::remove(_packagePath);
}

void UpdaterController::setStatus(const QString& text, double progress, bool indeterminate) {
  if (_statusText != text) {
    _statusText = text;
    emit statusTextChanged();
  }
  if (progress >= 0.0 && !qFuzzyCompare(_progress, progress)) {
    _progress = progress;
    emit progressChanged();
  }
  if (_indeterminate != indeterminate) {
    _indeterminate = indeterminate;
    emit indeterminateChanged();
  }
}

void UpdaterController::setError(const QString& error) {
  _parentTimer.stop();
  if (_process.state() != QProcess::NotRunning)
    _process.kill();
  _errorMessage = error;
  _failed = true;
  emit errorMessageChanged();
  emit failedChanged();
}
