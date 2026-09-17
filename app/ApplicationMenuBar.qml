import QtQuick
import QtQuick.Controls

MenuBar {
    id: root

    required property bool anyDialogOpen
    required property bool metricSelectorFocused
    required property var window

    signal newFileAction
    signal openFileAction
    signal saveFileDialogAction
    signal importFileDialogAction

    signal addAction
    signal editAction
    signal deleteAction
    signal rulesAction
    signal preferencesAction
    signal findOperationsAction
    signal previousPageAction(bool extendSelection)
    signal nextPageAction(bool extendSelection)
    signal previousUnbalancedCategoryAction
    signal nextUnbalancedCategoryAction
    signal previousEvolutionMetricAction
    signal nextEvolutionMetricAction
    signal saveAvailableAction
    signal reportAvailableAction

    signal checkUpdateAction
    signal projectPageAction
    signal aboutAction
    signal openRecentFileAction(string filePath)
    signal quitAction

    FileMenu {
        window: root.window

        onNewFileAction: root.newFileAction()
        onOpenFileAction: root.openFileAction()
        onSaveFileDialogAction: root.saveFileDialogAction()
        onImportFileDialogAction: root.importFileDialogAction()
        onOpenRecentFileAction: filePath => root.openRecentFileAction(filePath)
        onQuitAction: root.quitAction()
    }
    EditMenu {
        onAddAction: root.addAction()
        onEditAction: root.editAction()
        onDeleteAction: root.deleteAction()
        onRulesAction: root.rulesAction()
        onPreferencesAction: root.preferencesAction()
    }
    ViewMenu {
        anyDialogOpen: root.anyDialogOpen
        metricSelectorFocused: root.metricSelectorFocused
        onFindOperationsAction: root.findOperationsAction()
        onPreviousPageAction: extendSelection => root.previousPageAction(extendSelection)
        onNextPageAction: extendSelection => root.nextPageAction(extendSelection)
        onPreviousUnbalancedCategoryAction: root.previousUnbalancedCategoryAction()
        onNextUnbalancedCategoryAction: root.nextUnbalancedCategoryAction()
        onPreviousEvolutionMetricAction: root.previousEvolutionMetricAction()
        onNextEvolutionMetricAction: root.nextEvolutionMetricAction()
        onSaveAvailableAction: root.saveAvailableAction()
        onReportAvailableAction: root.reportAvailableAction()
    }
    HelpMenu {
        onCheckUpdateAction: root.checkUpdateAction()
        onProjectPageAction: root.projectPageAction()
        onAboutAction: root.aboutAction()
    }
}
