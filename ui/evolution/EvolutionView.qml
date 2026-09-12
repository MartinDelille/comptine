pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import ui.common
import services

FocusScope {
    id: root

    readonly property bool metricSelectorFocused: metricSelector.activeFocus || summaryStartSelector.activeFocus || summaryEndSelector.activeFocus

    readonly property int categoryColumnWidth: 180
    readonly property int monthColumnWidth: 125
    readonly property int summaryColumnWidth: 125
    readonly property int rowHeight: 36

    readonly property bool averageColumnVisible: averageCheckBox.checked
    readonly property bool sumColumnVisible: sumCheckBox.checked
    readonly property bool monthlySumRowVisible: monthlySumCheckBox.checked
    readonly property int horizontalHeaderHeight: rowHeight * (monthlySumRowVisible ? 2 : 1)
    readonly property int visibleFixedColumnWidth: categoryColumnWidth + (averageColumnVisible ? summaryColumnWidth : 0) + (sumColumnVisible ? summaryColumnWidth : 0)
    readonly property int availableMonthWidth: Math.max(0, tableFrame.width - visibleFixedColumnWidth)
    readonly property int visibleMonthCount: Math.max(1, Math.min(EvolutionController.monthCount, Math.floor(availableMonthWidth / monthColumnWidth)))
    readonly property int firstVisibleMonth: Math.max(0, Math.min(Math.max(0, EvolutionController.monthCount - visibleMonthCount), EvolutionController.currentMonthIndex - Math.floor(visibleMonthCount / 2)))

    function monthLabel(date) {
        return date?.toLocaleDateString(Qt.locale(), "MMMM yyyy") ?? "";
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingNormal

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: qsTr("Evolution")
                font.pixelSize: Theme.fontSizeXLarge
                font.bold: true
                color: Theme.textPrimary
            }

            ComboBox {
                id: metricSelector
                model: [qsTr("Budget"), qsTr("Spent"), qsTr("Leftover"), qsTr("Saved"), qsTr("Reported"), qsTr("Accumulated Leftover")]
                currentIndex: EvolutionController.selectedMetric
                onActivated: EvolutionController.selectedMetric = currentIndex
                Accessible.name: qsTr("Evolution metric")
            }

            CheckBox {
                id: averageCheckBox
                text: qsTr("Average Spent")
                checked: true
            }

            CheckBox {
                id: sumCheckBox
                text: qsTr("Sum Reported")
                checked: true
            }

            CheckBox {
                id: monthlySumCheckBox
                text: qsTr("Monthly Sum")
                checked: true
            }

            Label {
                text: qsTr("From")
                color: Theme.textSecondary
            }

            ComboBox {
                id: summaryStartSelector
                model: EvolutionController.availableMonthLabels
                currentIndex: EvolutionController.summaryStartIndex
                onActivated: EvolutionController.setSummaryStartMonthIndex(currentIndex)
                Accessible.name: qsTr("Summary start month")
            }

            Label {
                text: qsTr("To")
                color: Theme.textSecondary
            }

            ComboBox {
                id: summaryEndSelector
                model: EvolutionController.availableMonthLabels
                currentIndex: EvolutionController.summaryEndIndex
                onActivated: EvolutionController.setSummaryEndMonthIndex(currentIndex)
                Accessible.name: qsTr("Summary end month")
            }

            Item {
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: Theme.spacingSmall

                Label {
                    text: {
                        const monthCount = EvolutionController.monthCount;
                        if (monthCount === 0)
                            return "";
                        return qsTr("%1 months from %2 to %3").arg(monthCount).arg(root.monthLabel(EvolutionController.firstMonth)).arg(root.monthLabel(EvolutionController.lastMonth));
                    }
                    color: Theme.textSecondary
                }
            }
        }

        Rectangle {
            id: tableFrame
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.background
            border.color: Theme.border
            clip: true

            HorizontalHeaderView {
                id: horizontalHeader
                anchors.left: tableView.left
                anchors.top: parent.top
                anchors.right: parent.right
                height: root.rowHeight
                activeFocusOnTab: false
                syncView: tableView
                clip: true

                delegate: EvolutionMonthHeaderDelegate {
                    monthColumnWidth: root.monthColumnWidth
                    rowHeight: root.rowHeight
                    onMonthSelected: date => BudgetData.budgetDate = date
                }
            }

            HorizontalHeaderView {
                id: monthlySumHeader
                anchors.left: tableView.left
                anchors.top: horizontalHeader.bottom
                anchors.right: parent.right
                height: root.rowHeight
                visible: root.monthlySumRowVisible
                activeFocusOnTab: false
                syncView: tableView
                clip: true

                delegate: EvolutionMonthSumHeaderDelegate {
                    monthColumnWidth: root.monthColumnWidth
                    rowHeight: root.rowHeight
                    onMonthSelected: date => BudgetData.budgetDate = date
                }
            }

            Item {
                id: fixedHeaderArea
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: root.visibleFixedColumnWidth
                z: 1

                VerticalHeaderView {
                    id: categoryHeader
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.topMargin: root.horizontalHeaderHeight
                    anchors.bottom: parent.bottom
                    width: root.categoryColumnWidth
                    activeFocusOnTab: false
                    syncView: tableView
                    clip: true

                    delegate: EvolutionCategoryHeaderDelegate {
                        categoryColumnWidth: root.categoryColumnWidth
                        rowHeight: root.rowHeight
                        onCategorySelected: row => CategoryController.currentIndex = row
                    }
                }

                VerticalHeaderView {
                    id: averageHeader
                    x: root.categoryColumnWidth
                    anchors.top: parent.top
                    anchors.topMargin: root.horizontalHeaderHeight
                    anchors.bottom: parent.bottom
                    width: root.summaryColumnWidth
                    visible: root.averageColumnVisible
                    activeFocusOnTab: false
                    syncView: tableView
                    clip: true

                    delegate: EvolutionSummaryHeaderDelegate {
                        columnWidth: root.summaryColumnWidth
                        rowHeight: root.rowHeight
                        reportedMode: false
                        onCategorySelected: row => CategoryController.currentIndex = row
                    }
                }

                VerticalHeaderView {
                    id: sumHeader
                    x: root.categoryColumnWidth + (root.averageColumnVisible ? root.summaryColumnWidth : 0)
                    anchors.top: parent.top
                    anchors.topMargin: root.horizontalHeaderHeight
                    anchors.bottom: parent.bottom
                    width: root.summaryColumnWidth
                    visible: root.sumColumnVisible
                    activeFocusOnTab: false
                    syncView: tableView
                    clip: true

                    delegate: EvolutionSummaryHeaderDelegate {
                        columnWidth: root.summaryColumnWidth
                        rowHeight: root.rowHeight
                        reportedMode: true
                        onCategorySelected: row => CategoryController.currentIndex = row
                    }
                }

                Rectangle {
                    width: root.categoryColumnWidth
                    height: root.rowHeight
                    color: Theme.surface
                    border.color: Theme.border

                    Label {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingNormal
                        text: qsTr("Category")
                        verticalAlignment: Text.AlignVCenter
                        font.bold: true
                        color: Theme.textPrimary
                    }
                }

                Rectangle {
                    x: root.categoryColumnWidth
                    width: root.summaryColumnWidth
                    height: root.rowHeight
                    visible: root.averageColumnVisible
                    color: Theme.surface
                    border.color: Theme.border

                    Label {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingNormal
                        text: qsTr("Average Spent")
                        verticalAlignment: Text.AlignVCenter
                        font.bold: true
                        color: Theme.textPrimary
                    }
                }

                Rectangle {
                    x: root.categoryColumnWidth + (root.averageColumnVisible ? root.summaryColumnWidth : 0)
                    width: root.summaryColumnWidth
                    height: root.rowHeight
                    visible: root.sumColumnVisible
                    color: Theme.surface
                    border.color: Theme.border

                    Label {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingNormal
                        text: qsTr("Sum Reported")
                        verticalAlignment: Text.AlignVCenter
                        font.bold: true
                        color: Theme.textPrimary
                    }
                }

                Rectangle {
                    y: root.rowHeight
                    width: root.categoryColumnWidth
                    height: root.rowHeight
                    visible: root.monthlySumRowVisible
                    color: Theme.surface
                    border.color: Theme.border

                    Label {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingNormal
                        text: qsTr("Monthly Sum")
                        verticalAlignment: Text.AlignVCenter
                        font.bold: true
                        color: Theme.textPrimary
                    }
                }
            }

            TableView {
                id: tableView
                anchors.left: fixedHeaderArea.right
                anchors.top: monthlySumHeader.visible ? monthlySumHeader.bottom : horizontalHeader.bottom
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                model: EvolutionController
                clip: true
                reuseItems: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.VerticalFlick
                contentX: root.firstVisibleMonth * root.monthColumnWidth
                columnWidthProvider: function (column) {
                    return root.monthColumnWidth;
                }
                rowHeightProvider: function (row) {
                    return root.rowHeight;
                }

                delegate: EvolutionCellDelegate {
                    monthColumnWidth: root.monthColumnWidth
                    rowHeight: root.rowHeight
                    metricIndex: EvolutionController.selectedMetric
                    onActivated: (row, monthDate) => {
                        CategoryController.currentIndex = row;
                        BudgetData.budgetDate = monthDate;
                    }
                }

                ScrollBar.horizontal: ScrollBar {
                    policy: ScrollBar.AlwaysOff
                }
                ScrollBar.vertical: ScrollBar {}
            }

            Label {
                anchors.centerIn: parent
                visible: EvolutionController.count === 0
                text: qsTr("No categories defined")
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.textMuted
            }
        }
    }
}
