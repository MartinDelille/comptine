pragma ComponentBehavior: Bound

import QtQuick

import QtQuick.Controls

Menu {
    id: root
    required property var file
    required property var settings
    required property var window

    signal newFileAction
    signal openFileAction
    signal saveFileDialogAction
    signal importFileDialogAction
    signal openRecentFileAction(string filePath)
    signal quitAction

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
        onTriggered: {
            if (root.file.currentFilePath.length > 0) {
                root.file.saveToYamlFile(root.file.currentFilePath);
            } else {
                root.saveFileDialogAction();
            }
        }
    }
    Action {
        text: qsTr("&Save As...")
        shortcut: StandardKey.SaveAs
        onTriggered: root.saveFileDialogAction()
    }
    Menu {
        id: recentFilesMenu
        title: qsTr("Open &Recent")
        enabled: root.settings.recentFilesModel.rowCount() > 0

        Instantiator {
            model: root.settings.recentFilesModel
            delegate: MenuItem {
                required property var model
                text: model.display
                onTriggered: root.openRecentFileAction(model.display)
            }
            onObjectAdded: (index, object) => recentFilesMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => recentFilesMenu.removeItem(object)
        }

        MenuSeparator {
            visible: root.settings.recentFilesModel.rowCount() > 0
        }

        MenuItem {
            text: qsTr("Clear Recent Files")
            enabled: root.settings.recentFilesModel.rowCount() > 0
            onTriggered: root.settings.clearRecentFiles()
        }
    }
    MenuSeparator {}
    Action {
        text: qsTr("&Import CSV...")
        shortcut: "Ctrl+Shift+I"
        onTriggered: root.importFileDialogAction()
    }
    MenuSeparator {}
    Action {
        text: qsTr("&Quit")
        shortcut: StandardKey.Quit
        onTriggered: root.quitAction()
    }
}
