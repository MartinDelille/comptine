#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "TransactionModel.h"

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);

  TransactionModel transactionModel;

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("transactionModel", &transactionModel);
  
  QObject::connect(
      &engine,
      &QQmlApplicationEngine::objectCreationFailed,
      &app,
      []() { QCoreApplication::exit(-1); },
      Qt::QueuedConnection);
  engine.loadFromModule("Comptine", "Main");

  // Load CSV file if provided as command line argument
  if (argc > 1) {
    QString csvPath = QString::fromLocal8Bit(argv[1]);
    transactionModel.loadFromCsv(csvPath);
  }

  return app.exec();
}
