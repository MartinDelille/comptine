qt_add_qml_module(
  rules
  URI
  ui.rules
  STATIC
  NO_PLUGIN
  IMPORTS
  TARGET common
  TARGET services
  TARGET editor
  TARGET model
  QML_FILES
  RuleEditDialog.qml
  RulesView.qml
)
