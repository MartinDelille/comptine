#pragma once

#include <qqmlintegration.h>

#include "UpdaterController.h"
#include "utils/ForeignBase.h"

struct UpdaterControllerForeign : ForeignBase<UpdaterController> {
  Q_GADGET
  QML_FOREIGN(UpdaterController)
  QML_SINGLETON
  QML_NAMED_ELEMENT(UpdaterController)
};
