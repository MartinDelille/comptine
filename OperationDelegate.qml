import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var operation
    required property double balance
    required property bool selected
    required property bool alternate

    function formatAmount(amount) {
        return amount.toFixed(2).replace('.', ',') + " €";
    }

    width: parent ? parent.width : 0
    height: 50
    border.width: 1
    border.color: "#ddd"

    color: {
        if (root.selected)
            return "#e3f2fd";
        if (root.alternate)
            return "#f9f9f9";
        return "white";
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Label {
            text: root.operation?.date ?? ""
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 14
            color: "#333"
            Layout.preferredWidth: 100
        }

        Label {
            Layout.fillWidth: true
            text: root.operation?.description ?? ""
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            font.pixelSize: 14
            color: "#333"
        }

        Label {
            text: root.formatAmount(root.operation?.amount ?? 0)
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignRight
            font.pixelSize: 14
            color: (root.operation?.amount ?? 0) < 0 ? "#d32f2f" : "#388e3c"
            font.bold: true
            Layout.preferredWidth: 100
        }

        Label {
            text: root.formatAmount(root.balance)
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignRight
            font.pixelSize: 14
            color: root.balance < 0 ? "#d32f2f" : "#333"
            Layout.preferredWidth: 100
        }
    }
}
