set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(
  CommonUI
  URI
  commonui
  QML_FILES
  AmountField.qml
  BaseDialog.qml
  DateLabel.qml
  Theme.qml
)
