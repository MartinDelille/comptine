#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "BudgetData.h"

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);

  BudgetData budgetData;

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("budgetData", &budgetData);
  
  QObject::connect(
      &engine,
      &QQmlApplicationEngine::objectCreationFailed,
      &app,
      []() { QCoreApplication::exit(-1); },
      Qt::QueuedConnection);
  engine.loadFromModule("Comptine", "Main");

  // Load file if provided as command line argument
  if (argc > 1) {
    QString filePath = QString::fromLocal8Bit(argv[1]);
    if (filePath.endsWith(".yaml") || filePath.endsWith(".yml")) {
      budgetData.loadFromYaml(filePath);
    } else if (filePath.endsWith(".csv")) {
      budgetData.importFromCsv(filePath);
    }
  }

  return app.exec();
}
