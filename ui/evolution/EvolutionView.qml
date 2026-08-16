pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import ui.common
import services

FocusScope {
    id: root

    property int selectedMetric: 0
    property alias metricSelectorFocused: metricSelector.activeFocus

    readonly property int categoryColumnWidth: 180
    readonly property int monthColumnWidth: 125
    readonly property int rowHeight: 36

    readonly property int availableMonthWidth: Math.max(0, tableFrame.width - categoryColumnWidth)
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
                currentIndex: root.selectedMetric
                onActivated: root.selectedMetric = currentIndex
                Accessible.name: qsTr("Evolution metric")
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

            VerticalHeaderView {
                id: verticalHeader
                anchors.left: parent.left
                anchors.top: tableView.top
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

            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                width: root.categoryColumnWidth
                height: horizontalHeader.height
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

            TableView {
                id: tableView
                anchors.left: verticalHeader.right
                anchors.top: horizontalHeader.bottom
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
                    metricIndex: metricSelector.currentIndex
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
