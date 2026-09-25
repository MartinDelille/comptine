#include <QCoreApplication>
#include <QDateTime>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

#include "services/AppSettings.h"

using namespace Qt::StringLiterals;

class AppSettingsTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase() {
    QCoreApplication::setOrganizationName(u"Fictional Coverage Organization"_s);
    QCoreApplication::setApplicationName(u"Fictional AppSettings Test"_s);
    QSettings settings;
    settings.clear();
    settings.sync();
  }

  void defaultsAndPropertyPersistence() {
    AppSettings settings;
    QCOMPARE(settings.windowX(), 200);
    QCOMPARE(settings.windowY(), 200);
    QCOMPARE(settings.windowWidth(), 1200);
    QCOMPARE(settings.windowHeight(), 800);
    QCOMPARE(settings.language(), QString());
    QCOMPARE(settings.theme(), QString());
    QVERIFY(settings.checkForUpdates());
    QVERIFY(!settings.lastUpdateCheck().isValid());

    QSignalSpy languageSpy(&settings, &AppSettings::languageChangeRequested);
    settings.set_windowX(25);
    settings.set_windowY(35);
    settings.set_windowWidth(1024);
    settings.set_windowHeight(768);
    settings.set_language(u"fr"_s);
    settings.set_theme(u"dark"_s);
    settings.set_checkForUpdates(false);
    const QDateTime checkedAt(QDate(2026, 4, 5), QTime(10, 30));
    settings.set_lastUpdateCheck(checkedAt);
    QCOMPARE(languageSpy.count(), 1);

    AppSettings restored;
    QCOMPARE(restored.windowX(), 25);
    QCOMPARE(restored.windowY(), 35);
    QCOMPARE(restored.windowWidth(), 1024);
    QCOMPARE(restored.windowHeight(), 768);
    QCOMPARE(restored.language(), u"fr"_s);
    QCOMPARE(restored.theme(), u"dark"_s);
    QVERIFY(!restored.checkForUpdates());
    QCOMPARE(restored.lastUpdateCheck(), checkedAt);
  }

  void recentFilesAreUniqueOrderedBoundedAndPersisted() {
    AppSettings settings;
    QCOMPARE(settings.recentFiles().size(), 0);
    QVERIFY(settings.recentFilesModel() != nullptr);

    settings.addRecentFile(u"fictional-1.comptine"_s);
    settings.addRecentFile(u"fictional-2.comptine"_s);
    settings.addRecentFile(u"fictional-1.comptine"_s);
    QCOMPARE(settings.recentFiles(), QStringList({ "fictional-1.comptine"_L1, "fictional-2.comptine"_L1 }));

    for (int i = 3; i <= AppSettings::MaxRecentFiles + 2; ++i)
      settings.addRecentFile(u"fictional-%1.comptine"_s.arg(i));

    QCOMPARE(settings.recentFiles().size(), AppSettings::MaxRecentFiles);
    QCOMPARE(settings.recentFiles().first(), u"fictional-12.comptine"_s);
    QCOMPARE(settings.recentFiles().last(), u"fictional-3.comptine"_s);

    AppSettings restored;
    QCOMPARE(restored.recentFiles(), settings.recentFiles());

    settings.clearRecentFiles();
    QCOMPARE(settings.recentFiles().size(), 0);
    settings.clearRecentFiles();
    AppSettings cleared;
    QCOMPARE(cleared.recentFiles().size(), 0);
  }
};

QTEST_GUILESS_MAIN(AppSettingsTest)
#include "AppSettingsTest.moc"
