import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property int currentIndex

    // Get operation and balance from the model
    readonly property var operation: currentIndex >= 0 ? budgetData.operationModel.data(budgetData.operationModel.index(currentIndex, 0), 263) : null
    readonly property double balance: currentIndex >= 0 ? budgetData.operationModel.data(budgetData.operationModel.index(currentIndex, 0), 261) : 0

    // Multi-selection state
    readonly property bool multipleSelected: budgetData.operationModel.selectionCount > 1

    // Category list for ComboBox (reactive to category changes)
    property var categoryList: [""].concat(budgetData.categoryNames())

    Connections {
        target: budgetData
        function onCategoryCountChanged() {
            root.categoryList = [""].concat(budgetData.categoryNames());
        }
    }

    radius: Theme.cardRadius
    border.width: Theme.cardBorderWidth
    border.color: Theme.border
    color: Theme.surface

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingLarge
        spacing: Theme.spacingLarge

        Label {
            text: root.multipleSelected ? qsTr("Multiple Operations") : qsTr("Operation Details")
            font.pixelSize: Theme.fontSizeLarge
            font.bold: true
            color: Theme.textPrimary
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
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
                text: qsTr("%n operation(s)", "", budgetData.operationModel.selectionCount)
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
                text: Theme.formatAmount(budgetData.operationModel.selectedTotal)
                font.pixelSize: Theme.fontSizeLarge
                font.bold: true
                color: Theme.amountColor(budgetData.operationModel.selectedTotal)
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

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                Label {
                    id: budgetDateLabel
                    Layout.fillWidth: true
                    text: root.operation?.budgetDate ? root.operation.budgetDate.toLocaleDateString(Qt.locale(), Locale.LongFormat) : ""
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textPrimary
                    wrapMode: Text.WordWrap
                }

                ToolButton {
                    text: "✏️"
                    font.pixelSize: Theme.fontSizeSmall
                    opacity: hovered ? 1.0 : 0.5
                    onClicked: {
                        budgetDateDialog.operationIndex = root.currentIndex;
                        budgetDateDialog.originalBudgetDate = root.operation?.budgetDate ?? new Date();
                        budgetDateDialog.open();
                    }
                }
            }

            EditBudgetDateDialog {
                id: budgetDateDialog
            }

            Label {
                text: qsTr("Description:")
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.textSecondary
                Layout.topMargin: Theme.spacingSmall
            }

            Label {
                Layout.fillWidth: true
                text: root.operation?.description ?? ""
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textPrimary
                wrapMode: Text.WordWrap
            }

            Label {
                text: qsTr("Category:")
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.textSecondary
                Layout.topMargin: Theme.spacingSmall
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                ComboBox {
                    id: categoryComboBox
                    Layout.fillWidth: true
                    model: root.categoryList
                    currentIndex: {
                        if (!root.operation)
                            return 0;
                        let cat = root.operation.category ?? "";
                        if (cat === "")
                            return 0;
                        let idx = root.categoryList.indexOf(cat);
                        return idx >= 0 ? idx : 0;
                    }
                    displayText: currentIndex === 0 ? qsTr("Uncategorized") : currentText
                    onActivated: index => {
                        if (root.currentIndex >= 0) {
                            let newCategory = index === 0 ? "" : root.categoryList[index];
                            budgetData.setOperationCategory(root.currentIndex, newCategory);
                        }
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

            Label {
                text: qsTr("Balance:")
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.textSecondary
                Layout.topMargin: Theme.spacingSmall
            }

            Label {
                Layout.fillWidth: true
                text: root.operation ? Theme.formatAmount(root.balance) : ""
                font.pixelSize: Theme.fontSizeNormal
                font.bold: true
                color: Theme.balanceColor(root.balance)
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
