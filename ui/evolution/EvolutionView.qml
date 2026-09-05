pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import ui.common
import services

FocusScope {
    id: root

    property int selectedMetric: 0

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

            onWidthChanged: tableView.forceLayout()
            onHeightChanged: tableView.forceLayout()

            Connections {
                target: EvolutionController
                function onAvailableMonthsChanged() {
                    tableView.forceLayout();
                }
            }

            Connections {
                target: BudgetData
                function onBudgetDateChanged() {
                    tableView.forceLayout();
                }
            }

            HorizontalHeaderView {
                id: horizontalHeader
                anchors.left: tableView.left
                anchors.top: parent.top
                anchors.right: parent.right
                height: root.rowHeight
                syncView: tableView
                clip: true

                delegate: Rectangle {
                    id: horizontalDelegate
                    required property int column
                    required property date monthDate
                    required property bool currentMonth
                    implicitWidth: root.monthColumnWidth
                    implicitHeight: root.rowHeight
                    color: horizontalDelegate.currentMonth ? Theme.backgroundSelected : Theme.surface
                    border.color: Theme.border
                    border.width: 1

                    DateLabel {
                        anchors.fill: parent
                        date: horizontalDelegate.monthDate
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: Theme.fontSizeNormal
                        color: Theme.textPrimary
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: BudgetData.budgetDate = horizontalDelegate.monthDate
                    }
                }
            }

            VerticalHeaderView {
                id: verticalHeader
                anchors.left: parent.left
                anchors.top: tableView.top
                anchors.bottom: parent.bottom
                width: root.categoryColumnWidth
                syncView: tableView
                clip: true

                delegate: Rectangle {
                    id: verticalDelegate
                    required property int row
                    required property string categoryName
                    required property bool currentCategory
                    implicitWidth: root.categoryColumnWidth
                    implicitHeight: root.rowHeight
                    color: verticalDelegate.currentCategory ? Theme.backgroundSelected : (row % 2 === 0 ? Theme.background : Theme.backgroundAlt)
                    border.color: Theme.borderLight

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: CategoryController.currentIndex = verticalDelegate.row
                    }

                    Label {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingNormal
                        text: verticalDelegate.categoryName
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        color: Theme.textPrimary
                    }
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
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.VerticalFlick
                columnWidthProvider: function (column) {
                    if (column < root.firstVisibleMonth || column >= root.firstVisibleMonth + root.visibleMonthCount)
                        return 0;
                    return root.monthColumnWidth;
                }
                rowHeightProvider: function (row) {
                    return root.rowHeight;
                }

                delegate: Rectangle {
                    id: tableDelegate
                    required property int row
                    required property int column
                    required property real budget
                    required property real spent
                    required property real leftover
                    required property real saved
                    required property real reported
                    required property real accumulated
                    required property date monthDate
                    required property bool currentMonth
                    required property bool currentCategory
                    readonly property real cellValue: {
                        switch (metricSelector.currentIndex) {
                        case 0:
                            return tableDelegate.budget;
                        case 1:
                            return tableDelegate.spent;
                        case 2:
                            return tableDelegate.leftover;
                        case 3:
                            return tableDelegate.saved;
                        case 4:
                            return tableDelegate.reported;
                        case 5:
                            return tableDelegate.accumulated;
                        default:
                            return tableDelegate.budget;
                        }
                    }
                    implicitWidth: root.monthColumnWidth
                    implicitHeight: root.rowHeight
                    color: tableDelegate.currentCategory && tableDelegate.currentMonth ? Theme.backgroundSelectedStrong : (tableDelegate.currentCategory || tableDelegate.currentMonth ? Theme.backgroundSelected : (row % 2 === 0 ? Theme.background : Theme.backgroundAlt))
                    border.color: Theme.border
                    border.width: 1

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            CategoryController.currentIndex = tableDelegate.row;
                            BudgetData.budgetDate = tableDelegate.monthDate;
                        }
                    }

                    Label {
                        anchors.fill: parent
                        anchors.rightMargin: Theme.spacingSmall
                        text: Theme.formatAmount(tableDelegate.cellValue)
                        elide: Text.ElideNone
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                        font.bold: true
                        color: tableDelegate.cellValue >= 0 ? Theme.positive : Theme.negative
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
