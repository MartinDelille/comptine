#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "AppSettings.h"
#include "AppState.h"
#include "BudgetData.h"
#include "Foreigners.h"
#include "TranslationManager.h"
#include "UpdateController.h"

void comptineMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
  QByteArray localMsg = msg.toLocal8Bit();
  const char* file = context.file ? context.file : "";
  struct Prefix {
    const char* str;
    size_t len;
  };
  static Prefix prefixes[] = {
    { "qrc:/qt/qml/Comptine/", std::strlen("qrc:/qt/qml/Comptine/") },
    { PROJECT_ROOT_DIR "/", std::strlen(PROJECT_ROOT_DIR "/") }
  };
  for (const auto& p : prefixes) {
    if (file && std::strncmp(file, p.str, p.len) == 0) {
      file += p.len;
    }
  }
  int line = context.line;
  const char* colorStr = "";

  switch (type) {
    case QtDebugMsg:
      colorStr = "\033[32;1m";
      break;
    case QtWarningMsg:
      colorStr = "\033[35;1m";
      break;
    case QtCriticalMsg:
    case QtFatalMsg:
      colorStr = "\033[31;1m";
      break;
    case QtInfoMsg:
      colorStr = "\033[36;1m";
      break;
  }
  // Format: file:line: type: message
  fprintf(stderr, "%s:%d %s%s\033[0m\n", file, line, colorStr, localMsg.constData());
  if (type == QtFatalMsg) abort();
}

int main(int argc, char* argv[]) {
  qInstallMessageHandler(comptineMessageHandler);
  QGuiApplication app(argc, argv);
  app.setOrganizationDomain("martin.delille.org");
  app.setApplicationName("Comptine");

  QUndoStack undoStack;
  AppSettings settings;
  UpdateController updateController(settings);
  BudgetData budgetData(undoStack);
  CategoryController categories(budgetData, undoStack);
  RuleController rules(budgetData, undoStack);
  FileController file(settings, budgetData, categories, rules, undoStack);
  AccountEditor accountEditor(budgetData, undoStack);
  CategoryEditor categoryEditor(categories, budgetData, undoStack);
  OperationEditor operationEditor(budgetData, undoStack);
  RuleEditor ruleEditor(rules, budgetData, undoStack);
  ImportEditor importEditor(file);

  AppState appState;

  UndoStackForeign::instance = &undoStack;
  AppSettingsForeign::instance = &settings;
  UpdateControllerForeign::instance = &updateController;
  AppStateForeign::instance = &appState;
  BudgetDataForeign::instance = &budgetData;
  CategoryControllerForeign::instance = &categories;
  RuleControllerForeign::instance = &rules;
  FileControllerForeign::instance = &file;
  AccountEditorForeign::instance = &accountEditor;
  CategoryEditorForeign::instance = &categoryEditor;
  OperationEditorForeign::instance = &operationEditor;
  RuleEditorForeign::instance = &ruleEditor;
  ImportEditorForeign::instance = &importEditor;

  file.loadInitialFile(QCoreApplication::arguments());

  QQmlApplicationEngine engine;

  // Setup translation manager (handles initial load and live switching)
  TranslationManager translationManager(app, engine, settings);

  // Handle QML creation failure
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  engine.loadFromModule("Comptine", "Main");

  return app.exec();
}
