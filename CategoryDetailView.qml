import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    title: categoryName
    modal: true
    anchors.centerIn: parent
    width: 600
    height: 500
    standardButtons: Dialog.Close

    property string categoryName: ""
    property int year: 0
    property int month: 0
    property var operations: []
    property real totalAmount: 0

    onOpened: {
        operations = budgetData.operationsForCategory(categoryName, year, month);
        totalAmount = operations.reduce((sum, op) => sum + op.amount, 0);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingNormal

        // Header with month and total
        RowLayout {
            Layout.fillWidth: true

            Label {
                text: {
                    let date = new Date(year, month - 1, 1);
                    return date.toLocaleDateString(Qt.locale(), "MMMM yyyy");
                }
                font.pixelSize: Theme.fontSizeLarge
                font.bold: true
                color: Theme.textPrimary
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Total: %1").arg(Theme.formatAmount(Math.abs(totalAmount)))
                font.pixelSize: Theme.fontSizeNormal
                font.bold: true
                color: totalAmount >= 0 ? Theme.positive : Theme.negative
            }
        }

        // Operations count
        Label {
            text: qsTr("%n operation(s)", "", operations.length)
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.textMuted
        }

        // Operations list
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: operations
            clip: true
            spacing: Theme.spacingSmall

            delegate: Rectangle {
                required property var modelData
                required property int index

                width: ListView.view.width
                height: contentRow.implicitHeight + 16
                color: operationMouseArea.containsMouse ? Theme.borderLight : (index % 2 === 0 ? Theme.surface : Theme.surfaceElevated)
                radius: Theme.cardRadius

                MouseArea {
                    id: operationMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        budgetData.selectOperation(modelData.accountName, modelData.date, modelData.description, modelData.amount);
                        root.close();
                    }
                }

                RowLayout {
                    id: contentRow
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: Theme.spacingNormal

                    // Date
                    Label {
                        text: modelData.date.toLocaleDateString(Qt.locale(), "dd/MM")
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textMuted
                        Layout.preferredWidth: 50
                    }

                    // Description
                    Label {
                        text: modelData.description
                        font.pixelSize: Theme.fontSizeNormal
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    // Account name
                    Label {
                        text: modelData.accountName
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textMuted
                        Layout.preferredWidth: 80
                        horizontalAlignment: Text.AlignRight
                    }

                    // Amount
                    Label {
                        text: Theme.formatAmount(modelData.amount)
                        font.pixelSize: Theme.fontSizeNormal
                        font.bold: true
                        color: modelData.amount >= 0 ? Theme.positive : Theme.negative
                        horizontalAlignment: Text.AlignRight
                        Layout.preferredWidth: 100
                    }
                }
            }
        }

        // Empty state
        Label {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: operations.length === 0
            text: qsTr("No operations for this category")
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: Theme.fontSizeNormal
            color: Theme.textMuted
        }
    }
}
