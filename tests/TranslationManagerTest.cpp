#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTest>

#include "services/AppSettings.h"
#include "services/TranslationManager.h"

using namespace Qt::StringLiterals;

class TranslationManagerTest : public QObject {
  Q_OBJECT

private slots:
  void loadsSystemDefaultAndExplicitLanguages() {
    QQmlApplicationEngine engine;
    AppSettings settings;
    settings.set_language({});
    TranslationManager manager(*qobject_cast<QGuiApplication*>(QCoreApplication::instance()),
                               engine, settings);

    manager.loadTranslation();
    settings.set_language(u"fr"_s);
    QVERIFY(settings.language() == "fr"_L1);
    settings.set_language(u"en"_s);
    QVERIFY(settings.language() == "en"_L1);
    settings.set_language(u"fictional"_s);
    QVERIFY(settings.language() == "fictional"_L1);
    manager.loadTranslation();
  }
};

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen"_ba);
  QGuiApplication app(argc, argv);
  TranslationManagerTest test;
  return QTest::qExec(&test, argc, argv);
}

#include "TranslationManagerTest.moc"
