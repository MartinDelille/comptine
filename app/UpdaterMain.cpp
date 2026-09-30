#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "UpdaterController.h"

int main(int argc, char* argv[]) {
  QGuiApplication app(argc, argv);
  app.setApplicationName("ComptineUpdater");

  const QStringList arguments = app.arguments();
  if (arguments.size() != 7 || arguments[1] != "--executable" || arguments[3] != "--package" || arguments[5] != "--pid") {
    return 2;
  }

  bool pidOk = false;
  const qint64 parentPid = arguments[6].toLongLong(&pidOk);
  if (!pidOk)
    return 2;

  qInfo() << "Starting ComptineUpdater with arguments" << arguments;
  UpdaterController updater(arguments[2], arguments[4], parentPid);
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("updater", &updater);
  engine.loadFromModule("updater", "Updater");
  if (engine.rootObjects().isEmpty())
    return 3;

  qInfo() << "ComptineUpdater window loaded";
  QMetaObject::invokeMethod(&updater, &UpdaterController::start, Qt::QueuedConnection);
  return app.exec();
}
