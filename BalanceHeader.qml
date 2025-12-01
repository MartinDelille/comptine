import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property double balance
    required property int operationCount
    required property string accountName

    function formatAmount(amount) {
        return amount.toFixed(2).replace('.', ',') + " €";
    }

    Layout.fillWidth: true
    Layout.preferredHeight: 50
    color: "#f5f5f5"
    border.width: 1
    border.color: "#ddd"
    radius: 4

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10

        Label {
            text: root.accountName.length > 0 ? root.accountName : qsTr("No Account")
            font.pixelSize: 16
            font.bold: true
            color: "#333"
        }

        Label {
            text: qsTr("Balance:")
            font.pixelSize: 14
            color: "#666"
        }

        Label {
            text: root.formatAmount(root.balance)
            font.pixelSize: 20
            font.bold: true
            color: root.balance < 0 ? "#d32f2f" : "#388e3c"
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("%1 operations").arg(root.operationCount)
            font.pixelSize: 14
            color: "#666"
        }
    }
}
