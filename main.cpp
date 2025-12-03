#include <QDate>
#include <QFile>
#include <QGuiApplication>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QTranslator>
#include "AppSettings.h"
#include "BudgetData.h"

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  app.setOrganizationDomain("martin.delille.org");
  app.setApplicationName("Comptine");

  // Load translations based on language preference
  QTranslator translator;
  AppSettings appSettings;

  auto loadTranslation = [&translator, &app, &appSettings]() {
    // Remove existing translator if any
    app.removeTranslator(&translator);

    QString lang = appSettings.language();
    if (lang.isEmpty()) {
      // System default
      if (translator.load(QLocale(), "comptine", "_", ":/i18n")) {
        app.installTranslator(&translator);
      }
    } else if (lang == "fr") {
      if (translator.load(":/i18n/comptine_fr.qm")) {
        app.installTranslator(&translator);
      }
    }
    // If lang == "en", don't load any translator (English is source)
  };

  loadTranslation();

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

  // Save last opened file when data is loaded, and re-apply UI state for YAML loads only
  QObject::connect(&budgetData, &BudgetData::dataLoaded,
                   [&settings, &budgetData, &loadUiState]() {
                     if (!budgetData.currentFilePath().isEmpty()) {
                       settings.setValue("lastFile",
                                         budgetData.currentFilePath());
                       // Re-apply UI state after YAML loads (for selectedOperationIndex
                       // which needs valid operation count). Skip for CSV imports
                       // where the import sets the correct account/operation.
                       loadUiState();
                     }
                   });

  // Save or clear last file when currentFilePath changes (e.g., Save As or File > New)
  QObject::connect(&budgetData, &BudgetData::currentFilePathChanged,
                   [&settings, &budgetData]() {
                     if (!budgetData.currentFilePath().isEmpty()) {
                       settings.setValue("lastFile",
                                         budgetData.currentFilePath());
                     } else {
                       // Clear lastFile when File > New is used
                       settings.remove("lastFile");
                     }
                   });

  // Save UI state when app is about to quit
  QObject::connect(&app, &QGuiApplication::aboutToQuit, saveUiState);

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("budgetData", &budgetData);
  engine.rootContext()->setContextProperty("appSettings", &appSettings);

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
  engine.loadFromModule("Comptine", "Main");

  // Live language switching: reload translation and retranslate QML
  QObject::connect(&appSettings, &AppSettings::languageChangeRequested,
                   [&loadTranslation, &engine]() {
                     loadTranslation();
                     engine.retranslate();
                   });

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
