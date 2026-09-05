qt_add_qml_module(
  evolution
  URI
  ui.evolution
  STATIC
  NO_PLUGIN
  IMPORTS
  TARGET common
  TARGET services
  TARGET model
  QML_FILES
  EvolutionView.qml
)
