import QtQuick
import QtQuick.Controls

Menu {
    id: root
    required property var budgetData
    required property bool anyDialogOpen
    title: qsTr("&View")
    Action {
        text: qsTr("&Operations")
        shortcut: "Ctrl+1"
        onTriggered: root.budgetData.currentTabIndex = 0
    }
    Action {
        text: qsTr("&Budget")
        shortcut: "Ctrl+2"
        onTriggered: root.budgetData.currentTabIndex = 1
    }
    MenuSeparator {}
    Action {
        text: qsTr("&Previous Month")
        shortcut: "Left"
        enabled: root.budgetData.currentTabIndex === 1 && !root.anyDialogOpen
        onTriggered: root.budgetData.previousMonth()
    }
    Action {
        text: qsTr("&Next Month")
        shortcut: "Right"
        enabled: root.budgetData.currentTabIndex === 1 && !root.anyDialogOpen
        onTriggered: root.budgetData.nextMonth()
    }
}
