import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Comptine

Rectangle {
    id: root

    required property double balance
    required property int operationCount

    Layout.fillWidth: true
    Layout.preferredHeight: 50
    color: Theme.surface
    border.width: Theme.cardBorderWidth
    border.color: Theme.border
    radius: Theme.cardRadius

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingNormal

        Label {
            text: qsTr("Balance:")
            font.pixelSize: Theme.fontSizeNormal
            color: Theme.textSecondary
        }

        Label {
            text: Theme.formatAmount(root.balance)
            font.pixelSize: Theme.fontSizeXLarge
            font.bold: true
            color: Theme.balanceColor(root.balance)
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("%1 operations").arg(root.operationCount)
            font.pixelSize: Theme.fontSizeNormal
            color: Theme.textSecondary
        }
    }
}
