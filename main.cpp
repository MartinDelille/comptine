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

  // Set default budget year/month to current date (will be overridden when file loads)
  budgetData.set_budgetYear(QDate::currentDate().year());
  budgetData.set_budgetMonth(QDate::currentDate().month());

  // Save last opened file when YAML is loaded
  QObject::connect(&budgetData, &BudgetData::yamlFileLoaded,
                   [&settings, &budgetData]() {
                     if (!budgetData.currentFilePath().isEmpty()) {
                       settings.setValue("lastFile",
                                         budgetData.currentFilePath());
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
