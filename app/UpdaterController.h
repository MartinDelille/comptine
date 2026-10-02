#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTimer>

class UpdaterController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
  Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
  Q_PROPERTY(bool indeterminate READ indeterminate NOTIFY indeterminateChanged)
  Q_PROPERTY(bool finished READ finished NOTIFY finishedChanged)
  Q_PROPERTY(bool failed READ failed NOTIFY failedChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
  UpdaterController(QString executable, QString packagePath, qint64 parentPid);

  QString statusText() const { return _statusText; }
  double progress() const { return _progress; }
  bool indeterminate() const { return _indeterminate; }
  bool finished() const { return _finished; }
  bool failed() const { return _failed; }
  QString errorMessage() const { return _errorMessage; }

  Q_INVOKABLE void start();

signals:
  void statusTextChanged();
  void progressChanged();
  void indeterminateChanged();
  void finishedChanged();
  void failedChanged();
  void errorMessageChanged();

private slots:
  void checkParentProcess();
  void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void processError(QProcess::ProcessError error);

private:
  enum class Step {
    None,
    Mount,
    Verify,
    Copy,
    Detach,
    Install,
  };

  void setStatus(const QString& text, double progress = -1.0, bool indeterminate = true);
  void setError(const QString& error);
  void startProcess(const QString& program, const QStringList& arguments, Step step);
  void parentExited();
  void installLinuxUpdate();
  void finishSuccessfully();
  void relaunchApplication();
  void cleanup();
  bool processIsRunning() const;

  QString _executable;
  QString _packagePath;
  qint64 _parentPid;
  QString _appBundle;
  QString _installTarget;
  QString _mountPoint;
  QString _replacement;
  QProcess _process;
  QTimer _parentTimer;
  Step _step = Step::None;
  bool _copyRetried = false;
  QString _statusText;
  double _progress = 0.0;
  bool _indeterminate = true;
  bool _finished = false;
  bool _failed = false;
  QString _errorMessage;
};
