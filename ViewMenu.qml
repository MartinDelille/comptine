import QtQuick
import QtQuick.Controls

Menu {
    title: qsTr("&View")
    Action {
        text: qsTr("&Operations")
        shortcut: "Ctrl+1"
        onTriggered: AppState.navigation.currentTabIndex = 0
    }
    Action {
        text: qsTr("&Budget")
        shortcut: "Ctrl+2"
        onTriggered: AppState.navigation.currentTabIndex = 1
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
