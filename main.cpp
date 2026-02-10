#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "AppState.h"
#include "TranslationManager.h"

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

  QQmlApplicationEngine engine;

  // Get the AppState singleton created by QML engine
  auto* appState = engine.singletonInstance<AppState*>("Comptine", "AppState");
  Q_ASSERT(appState);

  // Setup translation manager (handles initial load and live switching)
  TranslationManager translationManager(app, engine, *appState->settings());

  // Handle QML creation failure
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  // Load initial file after QML engine is fully initialized
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &app,
      [appState](QObject* obj, const QUrl&) {
        if (obj) {
          appState->file()->loadInitialFile(QCoreApplication::arguments());
        }
      },
      Qt::QueuedConnection);

  engine.loadFromModule("Comptine", "Main");

  return app.exec();
}
