pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import commonui

BaseDialog {
    id: root

    required property var category
    required property date date
    property var operations: []
    property real totalAmount: operations.reduce((sum, op) => sum + op.amount, 0)

    signal navigateToOperation(var operation)

    title: category?.name || ""
    width: 600
    height: 500
    rejectButtonText: ""

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingNormal

        // Header with month and total
        RowLayout {
            Layout.fillWidth: true

            Label {
                text: root.date.toLocaleDateString(Qt.locale(), "MMMM yyyy")
                font.pixelSize: Theme.fontSizeLarge
                font.bold: true
                color: Theme.textPrimary
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Total: %1").arg(Theme.formatAmount(Math.abs(root.totalAmount)))
                font.pixelSize: Theme.fontSizeNormal
                font.bold: true
                color: root.totalAmount >= 0 ? Theme.positive : Theme.negative
            }
        }

        // Operations count
        Label {
            text: qsTr("%n operation(s)", "", root.operations.length)
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.textMuted
        }

        // Operations list
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.operations
            clip: true
            spacing: Theme.spacingSmall

            delegate: Rectangle {
                id: operationDelegate
                required property var operation
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
                        let operation = operationDelegate.operation;
                        root.navigateToOperation(operation);
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
                        text: operationDelegate.operation.date.toLocaleDateString(Qt.locale(), "dd/MM")
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textMuted
                        Layout.preferredWidth: 50
                    }

                    Label {
                        text: operationDelegate.operation.label
                        font.pixelSize: Theme.fontSizeNormal
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    // Account name
                    Label {
                        text: operationDelegate.operation.account.name
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textMuted
                        Layout.preferredWidth: 80
                        horizontalAlignment: Text.AlignRight
                    }

                    // Amount
                    AmountLabel {
                        amount: operationDelegate.operation.amountForCategory(root.category)
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
            visible: root.operations.length === 0
            text: qsTr("No operations for this category")
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: Theme.fontSizeNormal
            color: Theme.textMuted
        }
    }
}
