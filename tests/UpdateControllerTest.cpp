#include <QCoreApplication>
#include <QNetworkReply>
#include <QSignalSpy>
#include <QTest>

#include "services/AppSettings.h"
#include "services/UpdateController.h"

namespace {

class FakeReply : public QNetworkReply {
  Q_OBJECT

public:
  FakeReply(NetworkError error, QByteArray data) : _data(std::move(data)) {
    open(QIODevice::ReadOnly);
    if (error != NoError)
      setError(error, "Fictional network error");
  }

  void abort() override {}

protected:
  qint64 readData(char* buffer, qint64 maxSize) override {
    const qint64 available = _data.size() - _offset;
    const qint64 amount = qMin(maxSize, available);
    if (amount > 0) {
      memcpy(buffer, _data.constData() + _offset, static_cast<size_t>(amount));
      _offset += amount;
    }
    return amount;
  }

private:
  QByteArray _data;
  qint64 _offset = 0;
};

}  // namespace

class UpdateControllerTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase() {
    QCoreApplication::setOrganizationName("Fictional Coverage Organization");
    QCoreApplication::setApplicationName("Fictional UpdateController Test");
  }

  void autoCheckHonorsPreferenceAndOneDayInterval() {
    AppSettings settings;
    UpdateController controller(settings);

    settings.set_checkForUpdates(true);
    settings.set_lastUpdateCheck({});
    QVERIFY(controller.shouldAutoCheck());

    settings.set_lastUpdateCheck(QDateTime::currentDateTime().addSecs(-23 * 60 * 60));
    QVERIFY(!controller.shouldAutoCheck());
    settings.set_lastUpdateCheck(QDateTime::currentDateTime().addSecs(-25 * 60 * 60));
    QVERIFY(controller.shouldAutoCheck());

    settings.set_checkForUpdates(false);
    QVERIFY(!controller.shouldAutoCheck());
  }

  void markCheckedAndCurrentVersionAreExposed() {
    AppSettings settings;
    UpdateController controller(settings);
    settings.set_lastUpdateCheck({});

    controller.markUpdateChecked();
    QVERIFY(settings.lastUpdateCheck().isValid());
    QVERIFY(settings.lastUpdateCheck().secsTo(QDateTime::currentDateTime()) < 2);
    QVERIFY(!controller.currentVersion().isEmpty());
  }

  void networkResponsesUpdateControllerState() {
    AppSettings settings;
    UpdateController controller(settings);
    QSignalSpy completedSpy(&controller, &UpdateController::updateCheckCompleted);
    QSignalSpy failedSpy(&controller, &UpdateController::updateCheckFailed);

    auto* newer = new FakeReply(QNetworkReply::NoError,
                                R"({"tag_name":"99.0","body":"Fictional release notes"})");
    QVERIFY(QMetaObject::invokeMethod(&controller, "onNetworkReply", Qt::DirectConnection,
                                      Q_ARG(QNetworkReply*, newer)));
    QCOMPARE(completedSpy.count(), 1);
    QCOMPARE(controller.latestVersion(), QString("99.0"));
    QCOMPARE(controller.releaseNotes(), QString("Fictional release notes"));
    QVERIFY(controller.updateAvailable());

    auto* invalid = new FakeReply(QNetworkReply::NoError, "not-json");
    QVERIFY(QMetaObject::invokeMethod(&controller, "onNetworkReply", Qt::DirectConnection,
                                      Q_ARG(QNetworkReply*, invalid)));
    QCOMPARE(failedSpy.count(), 1);
    QVERIFY(!controller.errorMessage().isEmpty());

    auto* failed = new FakeReply(QNetworkReply::ConnectionRefusedError, {});
    QVERIFY(QMetaObject::invokeMethod(&controller, "onNetworkReply", Qt::DirectConnection,
                                      Q_ARG(QNetworkReply*, failed)));
    QCOMPARE(failedSpy.count(), 2);
  }

  void repeatedCheckIsIgnoredWhilePending() {
    AppSettings settings;
    UpdateController controller(settings);

    controller.checkForUpdates();
    QVERIFY(controller.checking());
    controller.checkForUpdates();
    QVERIFY(controller.checking());

    auto* reply = new FakeReply(QNetworkReply::NoError, "{}");
    QVERIFY(QMetaObject::invokeMethod(&controller, "onNetworkReply", Qt::DirectConnection,
                                      Q_ARG(QNetworkReply*, reply)));
    QVERIFY(!controller.checking());
  }
};

QTEST_GUILESS_MAIN(UpdateControllerTest)
#include "UpdateControllerTest.moc"
