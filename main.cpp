#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "AppState.h"
#include "TranslationManager.h"

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  app.setOrganizationDomain("martin.delille.org");
  app.setApplicationName("Comptine");

  QQmlApplicationEngine engine;

  // Get the AppState singleton created by QML engine
  auto *appState = engine.singletonInstance<AppState *>("Comptine", "AppState");
  Q_ASSERT(appState);

  // Setup translation manager (handles initial load and live switching)
  TranslationManager translationManager(&app, &engine, appState->settings());

  // Handle QML creation failure
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  // Load initial file after QML engine is fully initialized
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &app,
      [appState](QObject *obj, const QUrl &) {
        if (obj) {
          appState->file()->loadInitialFile(QCoreApplication::arguments());
        }
      },
      Qt::QueuedConnection);

  engine.loadFromModule("Comptine", "Main");

  return app.exec();
}
