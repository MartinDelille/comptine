import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Comptine

Rectangle {
    id: root

    required property int currentIndex

    // Signal to request opening edit dialog (handled by parent)
    signal editRequested(var operation)

    // Get operation from the model using helper methods
    readonly property var operation: currentIndex >= 0 ? AppState.data.operationModel.operationAt(currentIndex) : null

    // Multi-selection state
    readonly property bool multipleSelected: AppState.data.operationModel.selectionCount > 1

    radius: Theme.cardRadius
    border.width: Theme.cardBorderWidth
    border.color: Theme.border
    color: Theme.surface

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingLarge
        spacing: Theme.spacingLarge

        // Header with title and edit button
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal

            Label {
                text: root.multipleSelected ? qsTr("Multiple Operations") : qsTr("Operation Details")
                font.pixelSize: Theme.fontSizeLarge
                font.bold: true
                color: Theme.textPrimary
            }

            Item {
                Layout.fillWidth: true
            }

            ToolButton {
                text: "\u270F\uFE0F"
                font.pixelSize: Theme.fontSizeNormal
                focusPolicy: Qt.NoFocus
                visible: root.operation !== null && !root.multipleSelected
                opacity: hovered ? 1.0 : 0.5
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Edit operation...")
                onClicked: {
                    if (root.operation) {
                        root.editRequested(root.operation);
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.border
        }

        // Multi-selection summary
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal
            visible: root.multipleSelected

            Label {
                text: qsTr("Selected:")
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.textSecondary
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("%n operation(s)", "", AppState.data.operationModel.selectionCount)
                font.pixelSize: Theme.fontSizeNormal
                color: Theme.textPrimary
            }

            Label {
                text: qsTr("Total Amount:")
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.textSecondary
                Layout.topMargin: Theme.spacingSmall
            }

            Label {
                Layout.fillWidth: true
                text: Theme.formatAmount(AppState.data.operationModel.selectedTotal)
                font.pixelSize: Theme.fontSizeLarge
                font.bold: true
                color: Theme.amountColor(AppState.data.operationModel.selectedTotal)
            }
        }

        // Single operation details
        GridLayout {
            Layout.fillWidth: true
            columns: 1
            rowSpacing: Theme.spacingNormal
            visible: root.operation !== null && !root.multipleSelected

            Label {
                text: qsTr("Date:")
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.textSecondary
            }

            Label {
                Layout.fillWidth: true
                text: root.operation?.date ? root.operation.date.toLocaleDateString(Qt.locale(), Locale.LongFormat) : ""
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textPrimary
                wrapMode: Text.WordWrap
            }

            Label {
                text: qsTr("Budget Date:")
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.textSecondary
                Layout.topMargin: Theme.spacingSmall
            }

            Label {
                Layout.fillWidth: true
                text: root.operation?.budgetDate ? root.operation.budgetDate.toLocaleDateString(Qt.locale(), Locale.LongFormat) : ""
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textPrimary
                wrapMode: Text.WordWrap
            }

            Label {
                text: qsTr("Label:")
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.textSecondary
                Layout.topMargin: Theme.spacingSmall
            }

            Label {
                Layout.fillWidth: true
                text: root.operation?.label ?? ""
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textPrimary
                wrapMode: Text.WordWrap
            }

            Label {
                text: qsTr("Details:")
                visible: root.operation?.details !== ""
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.textSecondary
                Layout.topMargin: Theme.spacingSmall
            }

            Label {
                Layout.fillWidth: true
                visible: root.operation?.details !== ""
                text: root.operation?.details ?? ""
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textPrimary
                wrapMode: Text.WordWrap
            }

            Label {
                text: qsTr("Allocations:")
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.textSecondary
                Layout.topMargin: Theme.spacingSmall
            }

            // Allocations view
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                Repeater {
                    model: root.operation?.allocations

                    RowLayout {
                        id: allocationRow
                        Layout.fillWidth: true
                        spacing: Theme.spacingSmall

                        required property var modelData

                        Label {
                            Layout.fillWidth: true
                            text: allocationRow.modelData.category ? allocationRow.modelData.category.name : qsTr("Uncategorized")
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textPrimary
                            elide: Text.ElideRight
                        }

                        Label {
                            text: Theme.formatAmount(allocationRow.modelData.amount)
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            color: Theme.amountColor(allocationRow.modelData.amount)
                        }
                    }
                }

                // Fallback when no allocations exist
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSmall
                    visible: !root.operation?.allocations.length

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Uncategorized")
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                        font.italic: true
                        elide: Text.ElideRight
                    }

                    Label {
                        text: Theme.formatAmount(root.operation?.amount ?? 0)
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        color: Theme.amountColor(root.operation?.amount ?? 0)
                    }
                }
            }

            Label {
                text: qsTr("Amount:")
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.textSecondary
                Layout.topMargin: Theme.spacingSmall
            }

            Label {
                Layout.fillWidth: true
                text: root.operation ? Theme.formatAmount(root.operation.amount) : ""
                font.pixelSize: Theme.fontSizeNormal
                font.bold: true
                color: Theme.amountColor(root.operation?.amount ?? 0)
                wrapMode: Text.WordWrap
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: root.operation === null && !root.multipleSelected ? qsTr("Select an operation to view details") : ""
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.textMuted
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
        }
    }
}
