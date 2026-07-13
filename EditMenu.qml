import QtQuick

import QtQuick.Controls

Menu {
    id: root

    title: qsTr("&Edit")

    required property int currentTabIndex
    required property var undoStack
    required property var budgetData
    required property var categories

    signal addAction
    signal editAction
    signal deleteAction
    signal rulesAction
    signal preferencesAction

    Action {
        text: qsTr("&Undo")
        shortcut: StandardKey.Undo
        enabled: root.undoStack.canUndo
        onTriggered: root.undoStack.undo()
    }
    Action {
        text: qsTr("&Redo")
        shortcut: StandardKey.Redo
        enabled: root.undoStack.canRedo
        onTriggered: root.undoStack.redo()
    }
    MenuSeparator {}
    Action {
        text: qsTr("&Copy")
        shortcut: StandardKey.Copy
        enabled: root.budgetData.currentAccount?.selectionCount > 0
        onTriggered: root.budgetData.copySelectedOperations()
    }
    Action {
        text: qsTr("Select &All")
        shortcut: StandardKey.SelectAll
        enabled: root.currentTabIndex === 0 && root.budgetData.currentAccount?.count > 0
        onTriggered: root.budgetData.currentAccount.selectAll()
    }
    MenuSeparator {}
    Action {
        text: root.currentTabIndex === 0 ? qsTr("Add New Operation...") : qsTr("Add New Category...")
        shortcut: "Ctrl+Shift+N"
        onTriggered: root.addAction()
    }
    Action {
        text: root.currentTabIndex === 0 ? qsTr("Edit &Operation...") : qsTr("Edit &Category...")
        shortcut: "Ctrl+E"
        enabled: root.currentTabIndex === 0 ? root.budgetData.currentAccount?.selectionCount === 1 : root.categories.current
        onTriggered: root.editAction()
    }
    Action {
        text: root.currentTabIndex === 0 ? qsTr("Delete Operation") : qsTr("Delete Category")
        shortcut: "Ctrl+Backspace"
        enabled: root.currentTabIndex === 0 ? (root.budgetData.currentAccount?.selectionCount > 0) : root.categories.current
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
