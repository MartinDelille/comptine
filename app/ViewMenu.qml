import QtQuick
import QtQuick.Controls

import services

Menu {
    id: root

    required property bool anyDialogOpen

    title: qsTr("&View")

    Action {
        text: qsTr("&Operations")
        shortcut: "Ctrl+1"
        onTriggered: BudgetData.currentTabIndex = 0
    }
    Action {
        text: qsTr("&Budget")
        shortcut: "Ctrl+2"
        onTriggered: BudgetData.currentTabIndex = 1
    }
    MenuSeparator {}
    Action {
        text: qsTr("&Previous Month")
        shortcut: "Left"
        enabled: BudgetData.currentTabIndex === 1 && !root.anyDialogOpen
        onTriggered: BudgetData.previousMonth()
    }
    Action {
        text: qsTr("&Next Month")
        shortcut: "Right"
        enabled: BudgetData.currentTabIndex === 1 && !root.anyDialogOpen
        onTriggered: BudgetData.nextMonth()
    }
}
