pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import ui.common

Rectangle {
    id: root

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
    required property bool budgetLimitChange
    required property int metricIndex
    required property real monthColumnWidth
    required property real rowHeight

    signal activated(int row, date monthDate)

    readonly property real cellValue: {
        switch (root.metricIndex) {
        case 0:
            return root.budget;
        case 1:
            return root.spent;
        case 2:
            return root.leftover;
        case 3:
            return root.saved;
        case 4:
            return root.reported;
        case 5:
            return root.accumulated;
        default:
            return root.budget;
        }
    }

    implicitWidth: root.monthColumnWidth
    implicitHeight: root.rowHeight
    color: root.currentCategory && root.currentMonth ? Theme.backgroundSelectedStrong : (root.currentCategory || root.currentMonth ? Theme.backgroundSelected : (root.row % 2 === 0 ? Theme.background : Theme.backgroundAlt))
    border.color: Theme.border
    border.width: 1

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.activated(root.row, root.monthDate)
    }

    Label {
        anchors.fill: parent
        anchors.rightMargin: Theme.spacingSmall
        text: Theme.formatAmount(root.cellValue)
        elide: Text.ElideNone
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        font.bold: root.metricIndex === 0 && root.budgetLimitChange
        color: root.cellValue >= 0 ? Theme.positive : Theme.negative
    }
}
