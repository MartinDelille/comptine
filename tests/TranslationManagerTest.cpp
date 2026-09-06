#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTest>

#include "services/AppSettings.h"
#include "services/TranslationManager.h"

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
    settings.set_language("fr");
    QVERIFY(settings.language() == "fr");
    settings.set_language("en");
    QVERIFY(settings.language() == "en");
    settings.set_language("fictional");
    QVERIFY(settings.language() == "fictional");
    manager.loadTranslation();
  }
};

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QGuiApplication app(argc, argv);
  TranslationManagerTest test;
  return QTest::qExec(&test, argc, argv);
}

#include "TranslationManagerTest.moc"
