import QtQuick
import QtQuick.Controls

import services

Menu {
    id: root

    title: qsTr("&Edit")

    signal addAction
    signal editAction
    signal deleteAction
    signal rulesAction
    signal preferencesAction

    Action {
        text: qsTr("&Undo")
        shortcut: StandardKey.Undo
        enabled: UndoStack.canUndo
        onTriggered: UndoStack.undo()
    }
    Action {
        text: qsTr("&Redo")
        shortcut: StandardKey.Redo
        enabled: UndoStack.canRedo
        onTriggered: UndoStack.redo()
    }
    MenuSeparator {}
    Action {
        text: qsTr("&Copy")
        shortcut: StandardKey.Copy
        enabled: BudgetData.currentAccount?.selectionCount > 0
        onTriggered: BudgetData.copySelectedOperations()
    }
    Action {
        text: qsTr("Select &All")
        shortcut: StandardKey.SelectAll
        enabled: BudgetData.currentTabIndex === 0 && BudgetData.currentAccount?.count > 0
        onTriggered: BudgetData.currentAccount.selectAll()
    }
    MenuSeparator {}
    Action {
        text: BudgetData.currentTabIndex === 0 ? qsTr("Add New Operation...") : qsTr("Add New Category...")
        shortcut: "Ctrl+Shift+N"
        onTriggered: root.addAction()
    }
    Action {
        text: BudgetData.currentTabIndex === 0 ? qsTr("Edit &Operation...") : qsTr("Edit &Category...")
        shortcut: "Ctrl+E"
        enabled: BudgetData.currentTabIndex === 0 ? BudgetData.currentAccount?.selectionCount === 1 : CategoryController.current
        onTriggered: root.editAction()
    }
    Action {
        text: BudgetData.currentTabIndex === 0 ? qsTr("Delete Operation") : qsTr("Delete Category")
        shortcut: "Ctrl+Backspace"
        enabled: BudgetData.currentTabIndex === 0 ? (BudgetData.currentAccount?.selectionCount > 0) : CategoryController.current
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
