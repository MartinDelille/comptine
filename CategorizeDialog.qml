import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

BaseDialog {
    id: root
    title: qsTr("Categorize Operations")
    width: 600
    height: 450
    standardButtons: Dialog.Close

    // Enable Enter key to apply when a category is selected and there's a current operation
    canSubmit: categoryCombo.currentIndex >= 0 && currentOperation !== null

    // Override commitAndAccept to apply category instead of closing
    function commitAndAccept() {
        if (currentOperation && categoryCombo.currentText.length > 0) {
            applyCategoryAndNext(categoryCombo.currentText);
        }
    }

    // Applies a category to the current operation and loads the next
    function applyCategoryAndNext(category) {
        if (currentOperation && category && category.length > 0) {
            AppState.data.setOperationCategory(currentOperation, category);
            loadNextOperation();
        }
    }

    property var currentOperation: null
    property int currentIndex: 0
    property int totalCount: 0

    // Category list - refreshed on open
    property var categoryList: []

    // Track skipped operations and all uncategorized operations
    property var skippedOperations: []
    property var allUncategorized: []

    function loadNextOperation() {
        // Find next operation that hasn't been skipped
        for (let i = 0; i < allUncategorized.length; i++) {
            let op = allUncategorized[i];
            // Check if already categorized (by Apply) or skipped
            if (op.category.length === 0 && !op.isSplit && !isSkipped(op)) {
                currentOperation = op;
                currentIndex = i + 1;
                // Try to find a matching rule for suggestion
                let matchingCategory = AppState.rules.matchingCategoryForDescription(currentOperation.description);
                if (matchingCategory.length > 0) {
                    let catIndex = findCategoryIndex(matchingCategory);
                    if (catIndex >= 0) {
                        categoryCombo.currentIndex = catIndex;
                    } else {
                        categoryCombo.currentIndex = -1;
                    }
                } else {
                    categoryCombo.currentIndex = -1;
                }
                categoryCombo.forceActiveFocus();
                return;
            }
        }
        // No more operations to show
        currentOperation = null;
    }

    function isSkipped(op) {
        for (let i = 0; i < skippedOperations.length; i++) {
            if (skippedOperations[i] === op) {
                return true;
            }
        }
        return false;
    }

    function findCategoryIndex(name) {
        let cats = root.categoryList;
        for (let i = 0; i < cats.length; i++) {
            if (cats[i] === name)
                return i;
        }
        return -1;
    }

    onOpened: {
        // Refresh category list when dialog opens
        root.categoryList = AppState.categories.categoryNames();
        // Get all uncategorized operations and reset skipped list
        allUncategorized = AppState.rules.uncategorizedOperations();
        totalCount = allUncategorized.length;
        skippedOperations = [];
        loadNextOperation();
    }

    RuleEditDialog {
        id: ruleEditDialog
        onAccepted: {
            // After creating a rule, apply it to current operation and move to next
            if (currentOperation) {
                let matchingCategory = AppState.rules.matchingCategoryForDescription(currentOperation.description);
                if (matchingCategory.length > 0) {
                    applyCategoryAndNext(matchingCategory);
                } else {
                    loadNextOperation();
                }
            } else {
                loadNextOperation();
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingNormal

        // Progress indicator
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal

            Label {
                text: totalCount > 0 ? qsTr("Operation %1 of %2").arg(currentIndex).arg(totalCount) : qsTr("No uncategorized operations")
                font.pixelSize: Theme.fontSizeNormal
                color: Theme.textPrimary
            }

            Item {
                Layout.fillWidth: true
            }

            // Progress bar
            Rectangle {
                Layout.preferredWidth: 150
                Layout.preferredHeight: 8
                color: Theme.progressBackground
                radius: 4
                visible: totalCount > 0

                Rectangle {
                    width: totalCount > 0 ? ((currentIndex - 1) / totalCount) * parent.width : 0
                    height: parent.height
                    radius: 4
                    color: Theme.positive
                }
            }
        }

        // Operation details card
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 150
            color: Theme.surfaceElevated
            border.color: Theme.borderLight
            border.width: Theme.cardBorderWidth
            radius: Theme.cardRadius
            visible: currentOperation !== null

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingNormal
                spacing: Theme.spacingSmall

                // Date and amount row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingNormal

                    Label {
                        text: currentOperation ? Qt.formatDate(currentOperation.date, "dd MMM yyyy") : ""
                        font.pixelSize: Theme.fontSizeNormal
                        color: Theme.textSecondary
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Label {
                        text: currentOperation ? Theme.formatAmount(currentOperation.amount) : ""
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                        color: currentOperation && currentOperation.amount < 0 ? Theme.negative : Theme.positive
                    }
                }

                // Description
                Label {
                    Layout.fillWidth: true
                    text: currentOperation ? currentOperation.description : ""
                    font.pixelSize: Theme.fontSizeNormal
                    font.bold: true
                    color: Theme.textPrimary
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                    maximumLineCount: 2
                }

                // Reference
                Label {
                    Layout.fillWidth: true
                    text: currentOperation && currentOperation.reference && currentOperation.reference.length > 0 ? qsTr("Ref: %1").arg(currentOperation.reference) : ""
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textMuted
                    visible: text.length > 0
                }

                // Account
                Label {
                    text: currentOperation ? qsTr("Account: %1").arg(currentOperation.accountName) : ""
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                }
            }
        }

        // Empty state
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.surface
            border.color: Theme.borderLight
            border.width: Theme.cardBorderWidth
            radius: Theme.cardRadius
            visible: currentOperation === null

            ColumnLayout {
                anchors.centerIn: parent
                spacing: Theme.spacingNormal

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: "\u2713"
                    font.pixelSize: 48
                    color: Theme.positive
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("All operations are categorized!")
                    font.pixelSize: Theme.fontSizeLarge
                    color: Theme.textPrimary
                }
            }
        }

        // Category assignment section
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Assign Category")
            visible: currentOperation !== null

            ColumnLayout {
                anchors.fill: parent
                spacing: Theme.spacingNormal

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingNormal

                    ComboBox {
                        id: categoryCombo
                        Layout.fillWidth: true
                        model: root.categoryList
                        font.pixelSize: Theme.fontSizeNormal
                    }

                    Button {
                        text: qsTr("Apply")
                        enabled: categoryCombo.currentIndex >= 0
                        onClicked: {
                            if (currentOperation && categoryCombo.currentText.length > 0) {
                                applyCategoryAndNext(categoryCombo.currentText);
                            }
                        }
                    }
                }

                // Option to create a rule
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingNormal

                    Label {
                        text: qsTr("Or create a rule to auto-categorize similar operations:")
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Button {
                        text: qsTr("Create Rule...")
                        onClicked: {
                            if (currentOperation) {
                                // Suggest the description as prefix
                                ruleEditDialog.isNewRule = true;
                                ruleEditDialog.suggestedPrefix = currentOperation.description.substring(0, Math.min(20, currentOperation.description.length));
                                ruleEditDialog.open();
                            }
                        }
                    }
                }
            }
        }

        // Navigation buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal
            visible: currentOperation !== null

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Skip")
                onClicked: {
                    if (currentOperation) {
                        skippedOperations.push(currentOperation);
                    }
                    loadNextOperation();
                }
            }
        }
    }
}
