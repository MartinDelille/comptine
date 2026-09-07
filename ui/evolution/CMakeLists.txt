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
  EvolutionCellDelegate.qml
  EvolutionCategoryHeaderDelegate.qml
  EvolutionMonthHeaderDelegate.qml
  EvolutionMonthSumHeaderDelegate.qml
  EvolutionSummaryHeaderDelegate.qml
  EvolutionView.qml
)
