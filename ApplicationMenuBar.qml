import QtQuick
import QtQuick.Controls

MenuBar {
    id: root

    required property bool anyDialogOpen
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
    }
    HelpMenu {
        onCheckUpdateAction: root.checkUpdateAction()
        onProjectPageAction: root.projectPageAction()
        onAboutAction: root.aboutAction()
    }
}
