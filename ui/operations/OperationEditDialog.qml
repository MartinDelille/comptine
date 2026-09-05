pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import services
import editor
import ui.common
import ui.rules

BaseDialog {
    id: root

    property var _operation: null
    property var _unaffectedCategoryComboBox: null

    // Category list for ComboBoxes - refreshed on open
    property var categoryList: []

    title: qsTr("Edit Operation")
    width: Math.min(500, parent.width - 40)

    // Current edited amount (from AmountField)
    property double editedAmount: _operation?.amount ?? 0

    // Calculate remaining amount for allocations
    readonly property double remainingAmount: editedAmount - (_operation?.allocatedAmount ?? 0)

    okEnabled: labelField.text.trim() !== "" && amountField.value != 0

    onOpened: {
        // Refresh category list when dialog opens
        root.categoryList = CategoryController.categoryNames();
    }

    RuleEditDialog {
        id: ruleEditDialog
    }

    CreateCounterPartDialog {
        id: counterPartDialog
        operation: root._operation
        onCreateCounterPart: function (account, category) {
            root.applyChanges();
            OperationEditor.endEditing(true);
            let newOperation = OperationEditor.createCounterpart(root._operation, account, category);
            BudgetData.navigateToOperation(newOperation);
            root.initialize(newOperation);
        }
    }

    function initialize(operation) {
        const isNewOperation = operation === null;
        _unaffectedCategoryComboBox = null;

        _operation = isNewOperation ? OperationEditor.beginNew(new Date(), 0, "", "") : operation;

        editedAmount = _operation?.amount || 0;
        labelField.text = _operation?.label || "";
        if (labelField.text === "") {
            labelField.forceActiveFocus();
        }
        detailsField.text = _operation?.details || "";

        dateInput.selectedDate = _operation?.date || new Date();
        dateInput.resetModifierUnlock();
        dateInput.readOnly = true;
        budgetDateInput.selectedDate = _operation?.budgetDate || dateInput.selectedDate;

        if (!isNewOperation)
            OperationEditor.beginEditing(_operation);
        open();
    }

    function focusUnaffectedComboBox() {
        if (_unaffectedCategoryComboBox && labelField.text && amountField.value) {
            _unaffectedCategoryComboBox.forceActiveFocus();
        }
    }
    on_UnaffectedCategoryComboBoxChanged: Qt.callLater(focusUnaffectedComboBox)

    function applyChanges() {
        let newLabel = labelField.text.trim();
        let newDetails = detailsField.text.trim();

        OperationEditor.setLabel(_operation, newLabel);
        OperationEditor.setDetails(_operation, newDetails);
        OperationEditor.setAmount(_operation, editedAmount);
        OperationEditor.setBudgetDate(_operation, budgetDateInput.selectedDate);
        OperationEditor.normalizeAllocations(_operation);

        // Apply date change LAST since it sorts and changes the operation's index.
        OperationEditor.setDate(_operation, dateInput.selectedDate);
    }

    function goToOperation(operation) {
        if (operation) {
            applyChanges();
            OperationEditor.endEditing(true);
            BudgetData.navigateToOperation(operation);
            initialize(operation);
        }
    }

    onAccepted: {
        applyChanges();
        OperationEditor.endEditing(true);
    }

    onRejected: {
        OperationEditor.endEditing(false);
    }

    onClosed: {
        if (OperationEditor.editing)
            OperationEditor.endEditing(false);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingLarge

        // Label section
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal

            Label {
                text: qsTr("Label:")
                font.bold: true
                color: Theme.textSecondary
                Layout.preferredWidth: 100
            }

            TextField {
                id: labelField
                Layout.fillWidth: true
                placeholderText: qsTr("Enter label")
            }

            Button {
                icon.name: "globe"
                enabled: labelField.text.length > 0
                onClicked: Qt.openUrlExternally("https://www.google.com/search?q=" + encodeURIComponent(labelField.text))
            }
        }

        // Details section
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal

            Label {
                text: qsTr("Details:")
                font.bold: true
                color: Theme.textSecondary
                Layout.preferredWidth: 100
            }

            TextField {
                id: detailsField
                Layout.fillWidth: true
                placeholderText: qsTr("Enter details")
            }
        }

        // Amount section
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal

            Label {
                text: qsTr("Amount:")
                font.bold: true
                color: Theme.textSecondary
                Layout.preferredWidth: 100
            }

            AmountField {
                id: amountField
                Layout.fillWidth: true
                value: root.editedAmount
                onEdited: newValue => {
                    root.editedAmount = newValue;
                }
            }
        }

        // Date section
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal

            Label {
                text: qsTr("Date:")
                font.bold: true
                color: Theme.textSecondary
                Layout.preferredWidth: 100
            }

            DateInput {
                id: dateInput
                unlockOnModifierClick: true
            }
        }

        // Budget Date section
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal

            Label {
                text: qsTr("Budget Date:")
                font.bold: true
                color: Theme.textSecondary
                Layout.preferredWidth: 100
            }

            DateInput {
                id: budgetDateInput
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.border
        }

        // Navigation buttons for uncategorized operations
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal
            visible: root._operation !== null

            Button {
                text: qsTr("Previous Uncategorized")
                enabled: root._operation && RuleController.previousUncategorizedOperation(root._operation) !== null
                onClicked: root.goToOperation(RuleController.previousUncategorizedOperation(root._operation))
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Next Uncategorized")
                enabled: root._operation && RuleController.nextUncategorizedOperation(root._operation) !== null
                onClicked: root.goToOperation(RuleController.nextUncategorizedOperation(root._operation))
            }
        }

        // Category allocations header
        RowLayout {
            Layout.fillWidth: true

            Label {
                text: qsTr("Categories:")
                font.bold: true
                color: Theme.textSecondary
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Remaining:")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }

            AmountLabel {
                amount: root.remainingAmount
            }
        }

        // Allocations list
        ListView {
            id: allocationListView
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, maxListHeight)
            clip: contentHeight > maxListHeight

            // Maximum height: leave room for other dialog content (about 60% of window)
            readonly property real maxListHeight: root.parent ? root.parent.height * 0.4 : 300

            model: root._operation
            spacing: Theme.spacingSmall

            ScrollBar.vertical: ScrollBar {
                policy: allocationListView.contentHeight > allocationListView.maxListHeight ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
            }

            delegate: RowLayout {
                id: allocationDelegate
                width: parent?.width || 0
                spacing: Theme.spacingNormal

                required property int index
                required property var category
                required property double amount

                ComboBox {
                    id: categoryCombo
                    Layout.fillWidth: true
                    model: root.categoryList
                    currentIndex: {
                        if (!allocationDelegate.category)
                            return -1;
                        let idx = root.categoryList.indexOf(allocationDelegate.category.name);
                        return idx >= 0 ? idx : -1;
                    }
                    displayText: {
                        if (!allocationDelegate.category)
                            return qsTr("Select category...");
                        return currentIndex < 0 ? qsTr("Deleted category") : currentText;
                    }
                    onActivated: idx => {
                        OperationEditor.setAllocationCategory(root._operation, allocationDelegate.index, CategoryController.getCategoryByName(root.categoryList[idx]));
                    }
                    Component.onCompleted: {
                        if (!allocationDelegate.category) {
                            root._unaffectedCategoryComboBox = categoryCombo;
                        }
                    }
                }

                AmountField {
                    id: allocationAmountField
                    Layout.preferredWidth: 120
                    value: allocationDelegate.amount
                    onEdited: newValue => {
                        OperationEditor.setAllocationAmount(root._operation, allocationDelegate.index, newValue);
                    }
                }

                ToolButton {
                    text: "⚖️"
                    focusPolicy: Qt.NoFocus
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Balance to remaining amount")
                    onClicked: {
                        // Add the remaining amount to this allocation's amount
                        let newAmount = allocationDelegate.amount + root.remainingAmount;
                        OperationEditor.setAllocationAmount(root._operation, allocationDelegate.index, newAmount);
                    }
                }

                ToolButton {
                    text: "🗑️"
                    enabled: root._operation?.allocationCount > 0
                    opacity: enabled ? 1.0 : 0.3
                    focusPolicy: Qt.NoFocus
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Remove category")
                    onClicked: OperationEditor.removeAllocation(root._operation, allocationDelegate.index)
                }
            }
        }

        // Add allocation button
        Button {
            Layout.alignment: Qt.AlignLeft
            text: qsTr("+ Add Category")
            onClicked: OperationEditor.addAllocation(root._operation, null, root.remainingAmount)
        }

        RowLayout {
            Layout.fillWidth: true

            // Create rule button
            Button {
                text: qsTr("Create Rule...")
                visible: root._operation !== null
                onClicked: {
                    if (root._operation) {
                        ruleEditDialog.isNewRule = true;
                        ruleEditDialog.suggestedMatch = labelField.text.trim();
                        ruleEditDialog.suggestedAmount = root.editedAmount;
                        if (root._operation.allocatedCategoryNames.length > 0) {
                            ruleEditDialog.suggestedCategory = root._operation.allocatedCategoryNames[0];
                        } else {
                            ruleEditDialog.suggestedCategory = "";
                        }
                        ruleEditDialog.open();
                    }
                }
            }
            Button {
                text: qsTr("Create counter part...")
                onClicked: {
                    root.applyChanges();
                    counterPartDialog.open();
                }
            }
        }
    }
}
