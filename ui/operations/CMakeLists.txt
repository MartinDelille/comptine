qt_add_qml_module(
  operations
  URI
  ui.operations
  STATIC
  NO_PLUGIN
  IMPORTS
  TARGET common
  TARGET rules
  TARGET services
  TARGET editor
  TARGET model
  QML_FILES
  AccountComboBox.qml
  BalanceHeader.qml
  CreateCounterPartDialog.qml
  DateInput.qml
  OperationDelegate.qml
  OperationDetails.qml
  OperationEditDialog.qml
  OperationList.qml
  OperationView.qml
  RenameAccountDialog.qml
)
