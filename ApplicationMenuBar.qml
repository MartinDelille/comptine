import QtQuick
import QtQuick.Controls

MenuBar {
    id: root

    required property bool anyDialogOpen
    required property var file
    required property var budgetData
    required property var undoStack
    required property var settings
    required property var window
    required property var currentTabIndex
    required property var categories

    signal newFileAction
    signal openFileAction
    signal saveFileDialogAction
    signal importFileDialogAction

    signal addAction
    signal editAction
    signal deleteAction
    signal rulesAction
    signal preferencesAction

    signal checkUpdateAction
    signal projectPageAction
    signal aboutAction
    signal openRecentFileAction(string filePath)
    signal quitAction

    FileMenu {
        file: root.file
        settings: root.settings
        window: root.window

        onNewFileAction: root.newFileAction()
        onOpenFileAction: root.openFileAction()
        onSaveFileDialogAction: root.saveFileDialogAction()
        onImportFileDialogAction: root.importFileDialogAction()
        onOpenRecentFileAction: filePath => root.openRecentFileAction(filePath)
        onQuitAction: root.quitAction()
    }
    EditMenu {
        budgetData: root.budgetData
        undoStack: root.undoStack
        currentTabIndex: root.currentTabIndex
        categories: root.categories

        onAddAction: root.addAction()
        onEditAction: root.editAction()
        onDeleteAction: root.deleteAction()
        onRulesAction: root.rulesAction()
        onPreferencesAction: root.preferencesAction()
    }
    ViewMenu {}
    HelpMenu {
        onCheckUpdateAction: root.checkUpdateAction()
        onProjectPageAction: root.projectPageAction()
        onAboutAction: root.aboutAction()
    }
}
