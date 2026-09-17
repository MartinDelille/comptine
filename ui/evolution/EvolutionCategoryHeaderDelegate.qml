pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import ui.common

Rectangle {
    id: root

    required property int row
    required property string categoryName
    required property bool currentCategory
    required property real categoryColumnWidth
    required property real rowHeight

    signal categorySelected(int row)

    implicitWidth: root.categoryColumnWidth
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
        anchors.leftMargin: Theme.spacingNormal
        text: root.categoryName
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
        color: Theme.textPrimary
    }
}
