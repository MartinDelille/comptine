#include <QtQml/qqmlextensionplugin.h>
#include <QtQuickTest>

#include <QCoreApplication>
#include <QObject>

#include "services/AppSettings.h"
#include "services/ServiceForeigners.h"

Q_IMPORT_QML_PLUGIN(servicesPlugin)
Q_IMPORT_QML_PLUGIN(modelPlugin)
Q_IMPORT_QML_PLUGIN(editorPlugin)

class Setup : public QObject {
  Q_OBJECT

  AppSettings* _settings = nullptr;

public slots:
  void applicationAvailable() {
    QCoreApplication::setOrganizationName("ComptineTests");
    QCoreApplication::setOrganizationDomain("comptine.example");
    QCoreApplication::setApplicationName("qmltests");

    _settings = new AppSettings;
    AppSettingsForeign::instance = _settings;
  }

  void qmlEngineAvailable(QQmlEngine*) {}

public:
  ~Setup() override {
    delete _settings;
    AppSettingsForeign::instance = nullptr;
  }
};

QUICK_TEST_MAIN_WITH_SETUP(qmltests, Setup)

#include "qmltests.moc"
