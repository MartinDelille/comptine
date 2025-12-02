#include <QDate>
#include <QFile>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include "BudgetData.h"

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  app.setOrganizationDomain("org.delille.martin");
  app.setOrganizationName("Martin Delille");
  app.setApplicationName("Comptine");

  QSettings settings;
  BudgetData budgetData;

  // Load UI state from settings (before QML loads)
  auto loadUiState = [&settings, &budgetData]() {
    budgetData.set_currentAccountIndex(
        settings.value("ui/currentAccount", 0).toInt());
    budgetData.set_selectedOperationIndex(
        settings.value("ui/selectedOperation", 0).toInt());
    budgetData.set_currentTabIndex(settings.value("ui/currentTab", 0).toInt());
    budgetData.set_budgetYear(
        settings.value("ui/budgetYear", QDate::currentDate().year()).toInt());
    budgetData.set_budgetMonth(
        settings.value("ui/budgetMonth", QDate::currentDate().month()).toInt());
  };

  // Save UI state to settings
  auto saveUiState = [&settings, &budgetData]() {
    settings.setValue("ui/currentAccount", budgetData.currentAccountIndex());
    settings.setValue("ui/selectedOperation",
                      budgetData.selectedOperationIndex());
    settings.setValue("ui/currentTab", budgetData.currentTabIndex());
    settings.setValue("ui/budgetYear", budgetData.budgetYear());
    settings.setValue("ui/budgetMonth", budgetData.budgetMonth());
    settings.sync();
  };

  // Load UI state immediately (sets defaults before QML binds)
  loadUiState();

  // Save last opened file when data is loaded, and re-apply UI state
  QObject::connect(&budgetData, &BudgetData::dataLoaded,
                   [&settings, &budgetData, &loadUiState]() {
                     if (!budgetData.currentFilePath().isEmpty()) {
                       settings.setValue("lastFile",
                                         budgetData.currentFilePath());
                     }
                     // Re-apply UI state after data loads (for selectedOperationIndex
                     // which needs valid operation count)
                     loadUiState();
                   });

  // Save last file when currentFilePath changes (e.g., Save As)
  QObject::connect(&budgetData, &BudgetData::currentFilePathChanged,
                   [&settings, &budgetData]() {
                     if (!budgetData.currentFilePath().isEmpty()) {
                       settings.setValue("lastFile",
                                         budgetData.currentFilePath());
                     }
                   });

  // Save UI state when app is about to quit
  QObject::connect(&app, &QGuiApplication::aboutToQuit, saveUiState);

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("budgetData", &budgetData);

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
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
