import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var operation
    required property double balance

    function formatAmount(amount) {
        return amount.toFixed(2).replace('.', ',') + " €";
    }

    border.width: 1
    border.color: "#ddd"
    color: "#fafafa"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        Label {
            text: qsTr("Operation Details")
            font.pixelSize: 16
            font.bold: true
            color: "#333"
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#ddd"
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 1
            rowSpacing: 10
            visible: root.operation !== null

            Label {
                text: qsTr("Date:")
                font.pixelSize: 12
                font.bold: true
                color: "#666"
            }

            Label {
                Layout.fillWidth: true
                text: root.operation?.date ?? ""
                font.pixelSize: 12
                color: "#333"
                wrapMode: Text.WordWrap
            }

            Label {
                text: qsTr("Description:")
                font.pixelSize: 12
                font.bold: true
                color: "#666"
                Layout.topMargin: 5
            }

            Label {
                Layout.fillWidth: true
                text: root.operation?.description ?? ""
                font.pixelSize: 12
                color: "#333"
                wrapMode: Text.WordWrap
            }

            Label {
                text: qsTr("Category:")
                font.pixelSize: 12
                font.bold: true
                color: "#666"
                Layout.topMargin: 5
            }

            Label {
                Layout.fillWidth: true
                text: root.operation?.category ?? qsTr("Uncategorized")
                font.pixelSize: 12
                color: root.operation?.category ? "#333" : "#999"
                wrapMode: Text.WordWrap
            }

            Label {
                text: qsTr("Amount:")
                font.pixelSize: 12
                font.bold: true
                color: "#666"
                Layout.topMargin: 5
            }

            Label {
                Layout.fillWidth: true
                text: root.operation ? root.formatAmount(root.operation.amount) : ""
                font.pixelSize: 14
                font.bold: true
                color: (root.operation?.amount ?? 0) < 0 ? "#d32f2f" : "#388e3c"
                wrapMode: Text.WordWrap
            }

            Label {
                text: qsTr("Balance:")
                font.pixelSize: 12
                font.bold: true
                color: "#666"
                Layout.topMargin: 5
            }

            Label {
                Layout.fillWidth: true
                text: root.operation ? root.formatAmount(root.balance) : ""
                font.pixelSize: 14
                font.bold: true
                color: root.balance < 0 ? "#d32f2f" : "#333"
                wrapMode: Text.WordWrap
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: root.operation === null ? qsTr("Select an operation to view details") : ""
            font.pixelSize: 12
            color: "#999"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
        }
    }
}
