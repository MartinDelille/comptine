import QtQuick
import QtQuick.Controls.Basic

TabButton {
    id: root

    background: Rectangle {
        color: root.hovered ? Theme.backgroundHover : "transparent"

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 2
            color: root.checked ? Theme.accent : "transparent"
        }
    }

    contentItem: Text {
        text: root.text
        color: root.checked ? Theme.textPrimary : Theme.textSecondary
        font.weight: root.checked ? Font.Medium : Font.Normal
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
