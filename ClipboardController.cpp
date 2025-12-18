#include "ClipboardController.h"
#include <QClipboard>
#include <QGuiApplication>

#include "OperationListModel.h"

ClipboardController::ClipboardController(OperationListModel& operationModel) :
    _operationModel(operationModel) {
}

void ClipboardController::copySelectedOperations() const {
  QString csv = _operationModel.selectedOperationsAsCsv();
  if (!csv.isEmpty()) {
    QGuiApplication::clipboard()->setText(csv);
  }
}
