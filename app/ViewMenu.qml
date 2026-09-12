import QtQuick
import QtQuick.Controls

import services

Menu {
    id: root

    required property bool anyDialogOpen
    required property bool metricSelectorFocused

    signal findOperationsAction
    signal previousPageAction(bool extendSelection)
    signal nextPageAction(bool extendSelection)
    signal previousUnbalancedCategoryAction
    signal nextUnbalancedCategoryAction
    signal saveAvailableAction
    signal reportAvailableAction

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
    Action {
        text: qsTr("Find Operations")
        shortcut: StandardKey.Find
        enabled: (BudgetData.currentTabIndex === 0 || BudgetData.currentTabIndex === 1) && !root.anyDialogOpen
        onTriggered: root.findOperationsAction()
    }
    Action {
        text: qsTr("Previous Page")
        shortcut: "PgUp"
        enabled: (BudgetData.currentTabIndex === 0 || BudgetData.currentTabIndex === 1) && !root.anyDialogOpen
        onTriggered: root.previousPageAction(false)
    }
    Shortcut {
        sequence: "Shift+PgUp"
        enabled: (BudgetData.currentTabIndex === 0 || BudgetData.currentTabIndex === 1) && !root.anyDialogOpen
        onActivated: root.previousPageAction(true)
    }
    Action {
        text: qsTr("Next Page")
        shortcut: "PgDown"
        enabled: (BudgetData.currentTabIndex === 0 || BudgetData.currentTabIndex === 1) && !root.anyDialogOpen
        onTriggered: root.nextPageAction(false)
    }
    Shortcut {
        sequence: "Shift+PgDown"
        enabled: (BudgetData.currentTabIndex === 0 || BudgetData.currentTabIndex === 1) && !root.anyDialogOpen
        onActivated: root.nextPageAction(true)
    }
    Action {
        text: qsTr("Previous Unbalanced Category")
        shortcut: "Alt+Up"
        enabled: BudgetData.currentTabIndex === 1 && !root.anyDialogOpen
        onTriggered: root.previousUnbalancedCategoryAction()
    }
    Action {
        text: qsTr("Next Unbalanced Category")
        shortcut: "Alt+Down"
        enabled: BudgetData.currentTabIndex === 1 && !root.anyDialogOpen
        onTriggered: root.nextUnbalancedCategoryAction()
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
        text: qsTr("Save Available")
        shortcut: "S"
        enabled: BudgetData.currentTabIndex === 1 && !root.anyDialogOpen && CategoryController.current
        onTriggered: root.saveAvailableAction()
    }
    Action {
        text: qsTr("Report Available")
        shortcut: "R"
        enabled: BudgetData.currentTabIndex === 1 && !root.anyDialogOpen && CategoryController.current
        onTriggered: root.reportAvailableAction()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Previous Category")
        shortcut: BudgetData.currentTabIndex === 2 ? "Up" : ""
        enabled: (BudgetData.currentTabIndex === 1 || BudgetData.currentTabIndex === 2) && !root.anyDialogOpen && !root.metricSelectorFocused && CategoryController.currentIndex > 0
        onTriggered: CategoryController.currentIndex = CategoryController.currentIndex - 1
    }
    Action {
        text: qsTr("Next Category")
        shortcut: BudgetData.currentTabIndex === 2 ? "Down" : ""
        enabled: (BudgetData.currentTabIndex === 1 || BudgetData.currentTabIndex === 2) && !root.anyDialogOpen && !root.metricSelectorFocused && CategoryController.currentIndex < CategoryController.count - 1
        onTriggered: {
            if (CategoryController.currentIndex < 0)
                CategoryController.currentIndex = 0;
            else
                CategoryController.currentIndex = CategoryController.currentIndex + 1;
        }
    }
    Action {
        text: qsTr("First Category")
        shortcut: BudgetData.currentTabIndex === 2 ? "Home" : ""
        enabled: (BudgetData.currentTabIndex === 1 || BudgetData.currentTabIndex === 2) && !root.anyDialogOpen && !root.metricSelectorFocused && CategoryController.count > 0 && CategoryController.currentIndex > 0
        onTriggered: CategoryController.currentIndex = 0
    }
    Action {
        text: qsTr("Last Category")
        shortcut: BudgetData.currentTabIndex === 2 ? "End" : ""
        enabled: (BudgetData.currentTabIndex === 1 || BudgetData.currentTabIndex === 2) && !root.anyDialogOpen && !root.metricSelectorFocused && CategoryController.count > 0 && CategoryController.currentIndex < CategoryController.count - 1
        onTriggered: CategoryController.currentIndex = CategoryController.count - 1
    }
}
