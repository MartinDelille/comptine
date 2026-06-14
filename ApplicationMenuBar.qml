pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import Comptine

MenuBar {
    id: root

    required property bool anyDialogOpen
    signal newFileAction
    signal openFileAction
    signal saveFileAction(bool saveAs)
    signal openRecentFileAction(string filePath)
    signal importCsvAction
    signal quitAction
    signal addAction
    signal editAction
    signal rulesAction
    signal preferencesAction
    signal deleteAction
    signal checkUpdateAction
    signal projectPageAction
    signal aboutAction

    Menu {
        title: qsTr("&File")
        Action {
            text: qsTr("&New...")
            shortcut: StandardKey.New
            onTriggered: root.newFileAction()
        }
        Action {
            text: qsTr("&Open...")
            shortcut: StandardKey.Open
            onTriggered: root.openFileAction()
        }
        Action {
            text: qsTr("&Save")
            shortcut: StandardKey.Save
            onTriggered: root.saveFileAction(false)
        }
        Action {
            text: qsTr("&Save As...")
            shortcut: StandardKey.SaveAs
            onTriggered: root.saveFileAction(true)
        }
        Menu {
            id: recentFilesMenu
            title: qsTr("Open &Recent")
            enabled: AppState.settings.recentFilesModel.rowCount() > 0

            Instantiator {
                model: AppState.settings.recentFilesModel
                delegate: MenuItem {
                    required property var model
                    text: model.display
                    onTriggered: root.openRecentFileAction(model.display)
                }
                onObjectAdded: (index, object) => recentFilesMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => recentFilesMenu.removeItem(object)
            }

            MenuSeparator {
                visible: AppState.settings.recentFilesModel.rowCount() > 0
            }

            MenuItem {
                text: qsTr("Clear Recent Files")
                enabled: AppState.settings.recentFilesModel.rowCount() > 0
                onTriggered: AppState.settings.clearRecentFiles()
            }
        }
        MenuSeparator {}
        Action {
            text: qsTr("&Import CSV...")
            shortcut: "Ctrl+Shift+I"
            onTriggered: root.importCsvAction()
        }
        MenuSeparator {}
        Action {
            text: qsTr("&Quit")
            shortcut: StandardKey.Quit
            onTriggered: root.quitAction()
        }
    }
    Menu {
        title: qsTr("&Edit")
        Action {
            text: qsTr("&Undo")
            shortcut: StandardKey.Undo
            enabled: AppState.undoStack.canUndo
            onTriggered: AppState.data.undo()
        }
        Action {
            text: qsTr("&Redo")
            shortcut: StandardKey.Redo
            enabled: AppState.undoStack.canRedo
            onTriggered: AppState.data.redo()
        }
        MenuSeparator {}
        Action {
            text: qsTr("&Copy")
            shortcut: StandardKey.Copy
            enabled: AppState.data.operationModel.selectionCount > 0
            onTriggered: AppState.clipboard.copySelectedOperations()
        }
        Action {
            text: qsTr("Select &All")
            shortcut: StandardKey.SelectAll
            enabled: AppState.navigation.currentTabIndex === 0 && AppState.data.operationModel.count > 0
            onTriggered: AppState.navigation.currentAccount.selectAll()
        }
        MenuSeparator {}
        Action {
            text: AppState.navigation.currentTabIndex === 0 ? qsTr("Add New Operation...") : qsTr("Add New Category...")
            shortcut: "Ctrl+Shift+N"
            onTriggered: root.addAction()
        }
        Action {
            text: AppState.navigation.currentTabIndex === 0 ? qsTr("Edit &Operation...") : qsTr("Edit &Category...")
            shortcut: "Ctrl+E"
            enabled: (AppState.navigation.currentTabIndex === 0 && AppState.data.operationModel.selectionCount === 1) || (AppState.navigation.currentTabIndex === 1 && AppState.navigation.currentCategoryIndex >= 0)
            onTriggered: root.editAction()
        }
        Action {
            text: AppState.navigation.currentTabIndex === 0 ? qsTr("Delete Operation") : qsTr("Delete Category")
            shortcut: "Ctrl+Backspace"
            enabled: (AppState.navigation.currentTabIndex === 0 && AppState.data.operationModel.selectionCount > 0) || (AppState.navigation.currentTabIndex === 1 && AppState.navigation.currentCategoryIndex >= 0)
            onTriggered: root.deleteAction()
        }
        MenuSeparator {}
        Action {
            text: qsTr("Categorization &Rules...")
            onTriggered: root.rulesAction()
        }
        MenuSeparator {}
        Action {
            text: qsTr("&Preferences...")
            shortcut: StandardKey.Preferences
            onTriggered: root.preferencesAction()
        }
    }
    Menu {
        title: qsTr("&View")
        Action {
            text: qsTr("&Operations")
            shortcut: "Ctrl+1"
            onTriggered: AppState.navigation.showOperationsTab()
        }
        Action {
            text: qsTr("&Budget")
            shortcut: "Ctrl+2"
            onTriggered: AppState.navigation.showBudgetTab()
        }
        MenuSeparator {}
        Action {
            text: qsTr("&Previous Month")
            shortcut: "Left"
            enabled: AppState.navigation.currentTabIndex === 1 && !root.anyDialogOpen
            onTriggered: AppState.navigation.previousMonth()
        }
        Action {
            text: qsTr("&Next Month")
            shortcut: "Right"
            enabled: AppState.navigation.currentTabIndex === 1 && !root.anyDialogOpen
            onTriggered: AppState.navigation.nextMonth()
        }
    }
    Menu {
        title: qsTr("&Help")
        Action {
            text: qsTr("Check for &Updates...")
            onTriggered: root.checkUpdateAction()
        }
        Action {
            text: qsTr("&Project Page")
            onTriggered: root.projectPageAction()
        }
        MenuSeparator {}
        Action {
            text: qsTr("&About Comptine")
            onTriggered: root.aboutAction()
        }
    }
}
