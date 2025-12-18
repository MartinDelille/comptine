import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: root

    property var budgetSummary: []
    property bool dialogOpen: categoryEditDialog.visible || categoryDetailView.visible

    function editCurrentCategory() {
        if (AppState.navigation.currentCategoryIndex >= 0 && AppState.navigation.currentCategoryIndex < budgetSummary.length) {
            let category = budgetSummary[AppState.navigation.currentCategoryIndex];
            categoryEditDialog.originalName = category.name;
            categoryEditDialog.originalBudgetLimit = category.signedBudgetLimit;
            categoryEditDialog.open();
        }
    }

    function addCategory() {
        categoryEditDialog.originalName = "";
        categoryEditDialog.originalBudgetLimit = 0;
        categoryEditDialog.open();
    }

    // Forward focus to the category list
    onActiveFocusChanged: {
        if (activeFocus && budgetSummary.length > 0) {
            if (AppState.navigation.currentCategoryIndex < 0) {
                AppState.navigation.currentCategoryIndex = 0;
            }
            categoryListView.forceActiveFocus();
        }
    }

    function updateBudgetSummary() {
        budgetSummary = AppState.categories.monthlyBudgetSummary(AppState.navigation.budgetYear, AppState.navigation.budgetMonth);
        // Reset current index if out of bounds, or initialize to first item
        if (AppState.navigation.currentCategoryIndex < 0 && budgetSummary.length > 0) {
            AppState.navigation.currentCategoryIndex = 0;
        } else if (AppState.navigation.currentCategoryIndex >= budgetSummary.length) {
            AppState.navigation.currentCategoryIndex = budgetSummary.length > 0 ? 0 : -1;
        }
        // Restore scroll position to current category after model update
        if (AppState.navigation.currentCategoryIndex >= 0) {
            categoryListView.positionViewAtIndex(AppState.navigation.currentCategoryIndex, ListView.Contain);
        }
    }

    Component.onCompleted: {
        updateBudgetSummary();
    }

    Connections {
        target: AppState.file
        function onDataLoaded() {
            updateBudgetSummary();
        }
    }

    Connections {
        target: AppState.categories
        function onCategoryCountChanged() {
            updateBudgetSummary();
        }
    }

    Connections {
        target: AppState.data
        function onOperationDataChanged() {
            updateBudgetSummary();
        }
    }

    Connections {
        target: AppState.navigation
        function onBudgetYearChanged() {
            updateBudgetSummary();
        }
        function onBudgetMonthChanged() {
            updateBudgetSummary();
        }
    }

    CategoryEditDialog {
        id: categoryEditDialog
        onClosed: {
            categoryListView.forceActiveFocus();
        }
    }

    CategoryDetailView {
        id: categoryDetailView
    }

    LeftoverDialog {
        id: leftoverDialog
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.background

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingXLarge
            spacing: Theme.spacingXLarge

            // Month navigation
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal

                MonthSelector {
                    id: monthSelector
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Leftover...")
                    onClicked: leftoverDialog.open()
                }
            }

            ListView {
                id: categoryListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: budgetSummary
                spacing: Theme.spacingNormal
                clip: true
                focus: true
                currentIndex: AppState.navigation.currentCategoryIndex
                keyNavigationEnabled: false

                Keys.onUpPressed: {
                    AppState.navigation.previousCategory();
                    categoryListView.positionViewAtIndex(AppState.navigation.currentCategoryIndex, ListView.Contain);
                }

                Keys.onDownPressed: {
                    AppState.navigation.nextCategory();
                    categoryListView.positionViewAtIndex(AppState.navigation.currentCategoryIndex, ListView.Contain);
                }

                Keys.onReturnPressed: {
                    if (AppState.navigation.currentCategoryIndex >= 0 && AppState.navigation.currentCategoryIndex < budgetSummary.length) {
                        let category = AppState.categories.getCategory(AppState.navigation.currentCategoryIndex);
                        categoryDetailView.category = category;
                        categoryDetailView.year = AppState.navigation.budgetYear;
                        categoryDetailView.month = AppState.navigation.budgetMonth;
                        categoryDetailView.open();
                    }
                }

                delegate: Rectangle {
                    required property var modelData
                    required property int index

                    width: ListView.view.width
                    implicitHeight: contentColumn.implicitHeight + 24
                    color: delegateMouseArea.containsMouse ? Theme.surface : Theme.surfaceElevated
                    border.color: AppState.navigation.currentCategoryIndex === index ? Theme.accent : Theme.borderLight
                    border.width: AppState.navigation.currentCategoryIndex === index ? 2 : Theme.cardBorderWidth
                    radius: Theme.cardRadius

                    MouseArea {
                        id: delegateMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            AppState.navigation.currentCategoryIndex = index;
                            categoryListView.forceActiveFocus();
                            categoryDetailView.category = AppState.categories.getCategory(index);
                            categoryDetailView.year = AppState.navigation.budgetYear;
                            categoryDetailView.month = AppState.navigation.budgetMonth;
                            categoryDetailView.open();
                        }
                    }

                    ColumnLayout {
                        id: contentColumn
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: Theme.spacingSmall

                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                text: modelData.name
                                font.pixelSize: Theme.fontSizeNormal
                                font.bold: true
                                color: Theme.textPrimary
                            }

                            Label {
                                text: modelData.isIncome ? qsTr("(income)") : ""
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.textMuted
                            }

                            // Edit button
                            ToolButton {
                                text: "✏️"
                                font.pixelSize: Theme.fontSizeNormal
                                focusPolicy: Qt.NoFocus
                                opacity: hovered ? 1.0 : 0.5
                                onClicked: {
                                    AppState.navigation.currentCategoryIndex = index;
                                    categoryEditDialog.originalName = modelData.name;
                                    categoryEditDialog.originalBudgetLimit = modelData.signedBudgetLimit;
                                    categoryEditDialog.open();
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            Label {
                                text: {
                                    if (modelData.isIncome && modelData.percentUsed < 100)
                                        return qsTr("PENDING");
                                    if (!modelData.isIncome && modelData.percentUsed > 100)
                                        return qsTr("EXCEEDED");
                                    return "";
                                }
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: modelData.isIncome ? Theme.warning : Theme.negative
                            }

                            Label {
                                text: {
                                    let base = Theme.formatAmount(modelData.amount) + " / " + Theme.formatAmount(modelData.budgetLimit);
                                    if (modelData.accumulated > 0) {
                                        return base + " (+" + Theme.formatAmount(modelData.accumulated) + ")";
                                    }
                                    return base;
                                }
                                font.pixelSize: Theme.fontSizeNormal
                                color: Theme.textSecondary
                            }
                        }

                        // Custom progress bar using rectangles
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 16
                            color: Theme.progressBackground
                            radius: 4

                            Rectangle {
                                width: Math.min(modelData.percentUsed / 100, 1.0) * parent.width
                                height: parent.height
                                radius: 4
                                color: {
                                    if (modelData.isIncome) {
                                        // Income: green when complete, warning when pending
                                        return modelData.percentUsed >= 100 ? Theme.positive : Theme.warning;
                                    } else {
                                        // Expense: red when exceeded, warning when close
                                        if (modelData.percentUsed > 100)
                                            return Theme.negative;
                                        if (modelData.percentUsed > 80)
                                            return Theme.warning;
                                        return Theme.positive;
                                    }
                                }
                            }
                        }

                        Label {
                            text: {
                                if (modelData.isIncome) {
                                    return modelData.remaining > 0 ? qsTr("Expected: %1").arg(Theme.formatAmount(modelData.remaining)) : qsTr("Received: %1 extra").arg(Theme.formatAmount(-modelData.remaining));
                                } else {
                                    return modelData.remaining >= 0 ? qsTr("Remaining: %1").arg(Theme.formatAmount(modelData.remaining)) : qsTr("Exceeded: %1").arg(Theme.formatAmount(-modelData.remaining));
                                }
                            }
                            font.pixelSize: Theme.fontSizeSmall
                            color: {
                                if (modelData.isIncome) {
                                    return modelData.remaining > 0 ? Theme.warning : Theme.positive;
                                } else {
                                    return modelData.remaining >= 0 ? Theme.textSecondary : Theme.negative;
                                }
                            }
                        }
                    }
                }
            }

            // Empty state
            Label {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: budgetSummary.length === 0
                text: qsTr("No categories defined")
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.textMuted
            }
        }
    }
}
