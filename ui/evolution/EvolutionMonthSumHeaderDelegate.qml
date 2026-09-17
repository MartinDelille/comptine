pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import ui.common

Rectangle {
    id: root

    required property int column
    required property date monthDate
    required property real monthlySum
    required property bool currentMonth
    required property real monthColumnWidth
    required property real rowHeight

    signal monthSelected(date monthDate)

    implicitWidth: root.monthColumnWidth
    implicitHeight: root.rowHeight
    color: root.currentMonth ? Theme.backgroundSelected : Theme.surface
    border.color: Theme.border
    border.width: 1

    Label {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacingSmall
        anchors.rightMargin: Theme.spacingSmall
        text: Theme.formatAmount(root.monthlySum)
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        font.bold: true
        color: root.monthlySum >= 0 ? Theme.positive : Theme.negative
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.monthSelected(root.monthDate)
    }
}
