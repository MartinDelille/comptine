pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import ui.common

Rectangle {
    id: root

    required property int column
    required property date monthDate
    required property bool currentMonth
    required property real monthColumnWidth
    required property real rowHeight

    signal monthSelected(date monthDate)

    implicitWidth: root.monthColumnWidth
    implicitHeight: root.rowHeight
    color: root.currentMonth ? Theme.backgroundSelected : Theme.surface
    border.color: Theme.border
    border.width: 1

    DateLabel {
        anchors.fill: parent
        date: root.monthDate
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: Theme.fontSizeNormal
        color: Theme.textPrimary
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.monthSelected(root.monthDate)
    }
}
