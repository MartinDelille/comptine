import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: Theme.background

    property var budgetSummary: []

    function updateBudgetSummary() {
        budgetSummary = budgetData.monthlyBudgetSummary(budgetData.budgetYear, budgetData.budgetMonth);
    }

    Component.onCompleted: updateBudgetSummary()

    Connections {
        target: budgetData
        function onDataLoaded() {
            // Update MonthSelector to match loaded state
            monthSelector.selectedYear = budgetData.budgetYear;
            monthSelector.selectedMonth = budgetData.budgetMonth;
            updateBudgetSummary();
        }
        function onCategoryCountChanged() {
            updateBudgetSummary();
        }
        function onBudgetYearChanged() {
            monthSelector.selectedYear = budgetData.budgetYear;
            updateBudgetSummary();
        }
        function onBudgetMonthChanged() {
            monthSelector.selectedMonth = budgetData.budgetMonth;
            updateBudgetSummary();
        }
        function onOperationDataChanged() {
            updateBudgetSummary();
        }
    }

    EditCategoryDialog {
        id: editCategoryDialog
    }

    CategoryDetailView {
        id: categoryDetailView
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingXLarge
        spacing: Theme.spacingXLarge

        // Month navigation
        MonthSelector {
            id: monthSelector
            selectedYear: budgetData.budgetYear
            selectedMonth: budgetData.budgetMonth
            onMonthChanged: (year, month) => {
                budgetData.budgetYear = year;
                budgetData.budgetMonth = month;
            }
        }

        // Budget list
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: budgetSummary
            spacing: Theme.spacingNormal
            clip: true

            delegate: Rectangle {
                required property var modelData
                required property int index

                width: ListView.view.width
                implicitHeight: contentColumn.implicitHeight + 24
                color: delegateMouseArea.containsMouse ? Theme.surface : Theme.surfaceElevated
                border.color: Theme.borderLight
                border.width: Theme.cardBorderWidth
                radius: Theme.cardRadius

                MouseArea {
                    id: delegateMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        categoryDetailView.categoryName = modelData.name;
                        categoryDetailView.year = budgetData.budgetYear;
                        categoryDetailView.month = budgetData.budgetMonth;
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
                            opacity: hovered ? 1.0 : 0.5
                            onClicked: {
                                editCategoryDialog.originalName = modelData.name;
                                editCategoryDialog.originalBudgetLimit = modelData.signedBudgetLimit;
                                editCategoryDialog.open();
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
                            text: Theme.formatAmount(modelData.amount) + " / " + Theme.formatAmount(modelData.budgetLimit)
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
