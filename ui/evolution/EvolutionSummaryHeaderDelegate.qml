pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import ui.common

Rectangle {
    id: root

    required property int row
    required property real categoryAverage
    required property real categorySum
    required property bool sumMode
    required property bool currentCategory
    required property real columnWidth
    required property real rowHeight

    signal categorySelected(int row)

    implicitWidth: root.columnWidth
    implicitHeight: root.rowHeight
    color: root.currentCategory ? Theme.backgroundSelected : (root.row % 2 === 0 ? Theme.background : Theme.backgroundAlt)
    border.color: Theme.borderLight

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.categorySelected(root.row)
    }

    Label {
        anchors.fill: parent
        anchors.rightMargin: Theme.spacingSmall
        text: Theme.formatAmount(root.sumMode ? root.categorySum : root.categoryAverage)
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        color: Theme.textPrimary
    }
}
