qt_add_qml_module(
  budget
  URI
  ui.budget
  STATIC
  NO_PLUGIN
  IMPORTS
  TARGET common
  TARGET services
  TARGET editor
  TARGET model
  QML_FILES
  BudgetProgress.qml
  BudgetView.qml
  CategoryDetailView.qml
  CategoryEditDialog.qml
  MonthCategoryItem.qml
)
