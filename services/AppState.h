#pragma once

#include <QObject>

#include "Version.h"
#include "utils/PropertyMacros.h"

class AppState : public QObject {
  Q_OBJECT

  PROPERTY_CONSTANT(QString, appVersion, APP_VERSION_FULL)
  PROPERTY_CONSTANT(QString, appCommitHash, APP_COMMIT_HASH)
};
