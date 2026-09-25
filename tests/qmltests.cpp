#include <QtQml/qqmlextensionplugin.h>
#include <QtQuickTest>

#include <QCoreApplication>
#include <QObject>

#include "services/AppSettings.h"
#include "services/ServiceForeigners.h"

using namespace Qt::StringLiterals;

Q_IMPORT_QML_PLUGIN(servicesPlugin)
Q_IMPORT_QML_PLUGIN(modelPlugin)
Q_IMPORT_QML_PLUGIN(editorPlugin)

class Setup : public QObject {
  Q_OBJECT

  AppSettings* _settings = nullptr;

public slots:
  void applicationAvailable() {
    QCoreApplication::setOrganizationName(u"ComptineTests"_s);
    QCoreApplication::setOrganizationDomain(u"comptine.example"_s);
    QCoreApplication::setApplicationName(u"qmltests"_s);

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
