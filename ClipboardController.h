#pragma once

#include <QObject>

class OperationListModel;

class ClipboardController : public QObject {
  Q_OBJECT

public:
  explicit ClipboardController(OperationListModel& operationModel);

  // Copy selected operations to system clipboard as CSV
  Q_INVOKABLE void copySelectedOperations() const;

private:
  OperationListModel& _operationModel;
};
