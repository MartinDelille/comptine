#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QFile>
#include "BudgetData.h"

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  app.setOrganizationDomain("delille.martin");
  app.setOrganizationName("Martin Delille");
  app.setApplicationName("Comptine");

  QSettings settings;
  BudgetData budgetData;

  // Save last opened file when data is loaded
  QObject::connect(&budgetData, &BudgetData::dataLoaded, [&settings, &budgetData]() {
    if (!budgetData.currentFilePath().isEmpty()) {
      settings.setValue("lastFile", budgetData.currentFilePath());
    }
  });

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("budgetData", &budgetData);
  
  QObject::connect(
      &engine,
      &QQmlApplicationEngine::objectCreationFailed,
      &app,
      []() { QCoreApplication::exit(-1); },
      Qt::QueuedConnection);
  engine.loadFromModule("Comptine", "Main");

  // Load file: command line argument takes priority, otherwise use last opened file
  if (argc > 1) {
    QString filePath = QString::fromLocal8Bit(argv[1]);
    if (filePath.endsWith(".yaml") || filePath.endsWith(".yml")) {
      budgetData.loadFromYaml(filePath);
    } else if (filePath.endsWith(".csv")) {
      budgetData.importFromCsv(filePath);
    }
  } else {
    QString lastFile = settings.value("lastFile").toString();
    if (!lastFile.isEmpty() && QFile::exists(lastFile)) {
      budgetData.loadFromYaml(lastFile);
    }
  }

  return app.exec();
}
