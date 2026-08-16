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
    Action {
        text: qsTr("&Evolution")
        shortcut: "Ctrl+3"
        onTriggered: BudgetData.currentTabIndex = 2
    }
    MenuSeparator {}
    Action {
        text: qsTr("&Previous Month")
        shortcut: "Left"
        enabled: (BudgetData.currentTabIndex === 1 || BudgetData.currentTabIndex === 2) && !root.anyDialogOpen
        onTriggered: BudgetData.previousMonth()
    }
    Action {
        text: qsTr("&Next Month")
        shortcut: "Right"
        enabled: (BudgetData.currentTabIndex === 1 || BudgetData.currentTabIndex === 2) && !root.anyDialogOpen
        onTriggered: BudgetData.nextMonth()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Previous Category")
        shortcut: BudgetData.currentTabIndex === 2 ? "Up" : ""
        enabled: (BudgetData.currentTabIndex === 1 || BudgetData.currentTabIndex === 2) && !root.anyDialogOpen && CategoryController.currentIndex > 0
        onTriggered: CategoryController.currentIndex = CategoryController.currentIndex - 1
    }
    Action {
        text: qsTr("Next Category")
        shortcut: BudgetData.currentTabIndex === 2 ? "Down" : ""
        enabled: (BudgetData.currentTabIndex === 1 || BudgetData.currentTabIndex === 2) && !root.anyDialogOpen && CategoryController.currentIndex < CategoryController.count - 1
        onTriggered: {
            if (CategoryController.currentIndex < 0)
                CategoryController.currentIndex = 0;
            else
                CategoryController.currentIndex = CategoryController.currentIndex + 1;
        }
    }
}
