import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

BaseDialog {
    id: root

    property var _operation: null

    // Original values (for tracking changes)
    property double originalAmount: 0
    property date originalDate: new Date()
    property date originalBudgetDate: new Date()
    property string originalLabel: ""
    property string originalDetails: ""
    property var originalAllocations: []
    property var _unaffectedCategoryComboBox: null

    // Category list for ComboBoxes - refreshed on open
    property var categoryList: []

    title: qsTr("Edit Operation")
    width: Math.min(500, parent.width - 40)

    // Current edited amount (from AmountField)
    property double editedAmount: originalAmount

    // Calculate remaining amount for allocations
    readonly property double allocatedAmount: {
        let sum = 0;
        for (let i = 0; i < allocationModel.count; i++) {
            sum += allocationModel.get(i).amount;
        }
        return sum;
    }
    readonly property double remainingAmount: editedAmount - allocatedAmount

    okEnabled: labelField.text.trim() !== "" && amountField.value != 0

    onOpened: {
        // Refresh category list when dialog opens
        root.categoryList = [""].concat(AppState.categories.categoryNames());
    }

    ListModel {
        id: allocationModel
    }

    RuleEditDialog {
        id: ruleEditDialog
    }

    // React to external changes to the operation's allocations
    // (e.g. when a rule is applied from RuleEditDialog)
    Connections {
        target: _operation
        function onAllocationsChanged() {
            refreshAllocations();
        }
    }

    CreateCounterPartDialog {
        id: counterPartDialog
        operation: _operation
        onCreateCounterPart: function (account) {
            let newOperation = AppState.data.createCounterPart(operation, account);
            // AppState.navigation.currentAccount = account;
            AppState.navigation.currentOperation = newOperation;
            AppState.navigation.navigateToOperation(newOperation);
            initialize(newOperation);
        }
    }

    function refreshAllocations() {
        if (!_operation)
            return;

        originalAllocations = [];
        allocationModel.clear();

        if (_operation.allocations && _operation.allocations.length > 0) {
            for (let i = 0; i < _operation.allocations.length; i++) {
                let catName = _operation.allocations[i].category ? _operation.allocations[i].category.name : "";
                let amt = _operation.allocations[i].amount;
                originalAllocations.push({
                    category: catName,
                    amount: amt
                });
                allocationModel.append({
                    category: catName,
                    amount: amt
                });
            }
        } else {
            allocationModel.append({
                category: _operation.category?.name ?? "",
                amount: _operation.amount || 0
            });
        }
    }

    function initialize(operation) {
        _unaffectedCategoryComboBox = null;

        _operation = operation;

        originalAmount = editedAmount = operation?.amount || 0;
        originalDate = operation?.date || new Date();
        originalBudgetDate = operation?.budgetDate || new Date();
        labelField.text = originalLabel = operation?.label || "";
        if (originalLabel === "") {
            labelField.forceActiveFocus();
        }
        detailsField.text = originalDetails = operation?.details || "";

        dateInput.selectedDate = originalDate;
        budgetDateInput.selectedDate = originalBudgetDate;

        refreshAllocations();
        open();
    }

    function addAllocation() {
        allocationModel.append({
            category: "",
            amount: remainingAmount
        });
    }

    function removeAllocation(index) {
        if (allocationModel.count > 1) {
            allocationModel.remove(index);
        }
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

        // Build allocations array
        let allocations = [];
        for (let i = 0; i < allocationModel.count; i++) {
            let item = allocationModel.get(i);
            if (item.category !== "" && Math.abs(item.amount) > 0.001) {
                allocations.push({
                    category: item.category,
                    amount: item.amount
                });
            }
        }

        if (_operation === null) {
            AppState.data.addOperation(dateInput.selectedDate, editedAmount, newLabel, newDetails, allocations);
            return;
        }

        if (newLabel !== originalLabel) {
            AppState.data.setOperationLabel(_operation, newLabel);
        }

        if (newDetails !== originalDetails) {
            AppState.data.setOperationDetails(_operation, newDetails);
        }

        // Apply amount change if different
        if (Math.abs(editedAmount - originalAmount) > 0.001) {
            AppState.data.setOperationAmount(_operation, editedAmount);
        }

        // Apply budget date change if different
        if (budgetDateInput.selectedDate.getTime() !== originalBudgetDate.getTime()) {
            AppState.data.setOperationBudgetDate(_operation, budgetDateInput.selectedDate);
        }

        if (allocations.length > 0) {
            // Check if allocations changed
            let allocationsChanged = false;
            if (originalAllocations.length !== allocations.length) {
                allocationsChanged = true;
            } else {
                // Compare allocations
                for (let i = 0; i < allocations.length; i++) {
                    if (allocations[i].category !== originalAllocations[i].category || Math.abs(allocations[i].amount - originalAllocations[i].amount) > 0.001) {
                        allocationsChanged = true;
                        break;
                    }
                }
            }
            if (allocationsChanged) {
                AppState.data.setOperationAllocations(_operation, allocations);
            }
        }

        // Apply date change LAST (since it sorts and changes the operation's index)
        if (dateInput.selectedDate.getTime() !== originalDate.getTime()) {
            AppState.data.setOperationDate(_operation, dateInput.selectedDate);
        }
    }

    // Navigate to previous uncategorized operation
    function goToPreviousUncategorized() {
        let prevOp = AppState.rules.previousUncategorizedOperation(_operation);
        if (prevOp) {
            applyChanges();
            initialize(prevOp);
        }
    }

    // Navigate to next uncategorized operation
    function goToNextUncategorized() {
        let nextOp = AppState.rules.nextUncategorizedOperation(_operation);
        if (nextOp) {
            applyChanges();
            initialize(nextOp);
        }
    }

    onAccepted: {
        applyChanges();
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
                    // When there's only one allocation (single category operation),
                    // automatically update its amount to match the total
                    if (allocationModel.count === 1) {
                        allocationModel.setProperty(0, "amount", newValue);
                    }
                }
                onLiveEdited: newValue => {
                    root.editedAmount = newValue;
                    // When there's only one allocation (single category operation),
                    // automatically update its amount to match the total
                    if (allocationModel.count === 1) {
                        allocationModel.setProperty(0, "amount", newValue);
                    }
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
            height: 1
            color: Theme.border
        }

        // Navigation buttons for uncategorized operations
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal
            visible: _operation !== null

            Button {
                text: qsTr("Previous Uncategorized")
                enabled: _operation && AppState.rules.previousUncategorizedOperation(_operation) !== null
                onClicked: root.goToPreviousUncategorized()
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Next Uncategorized")
                enabled: _operation && AppState.rules.nextUncategorizedOperation(_operation) !== null
                onClicked: root.goToNextUncategorized()
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

            Label {
                text: Theme.formatAmount(root.remainingAmount)
                font.bold: true
                color: Math.abs(root.remainingAmount) < 0.01 ? Theme.positive : Theme.warning
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

            model: allocationModel
            spacing: Theme.spacingSmall

            ScrollBar.vertical: ScrollBar {
                policy: allocationListView.contentHeight > allocationListView.maxListHeight ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
            }

            delegate: RowLayout {
                width: allocationListView.width
                spacing: Theme.spacingNormal

                required property int index
                required property string category
                required property double amount

                ComboBox {
                    id: categoryCombo
                    Layout.fillWidth: true
                    model: root.categoryList
                    currentIndex: {
                        if (category === "")
                            return 0;
                        let idx = root.categoryList.indexOf(category);
                        return idx >= 0 ? idx : 0;
                    }
                    displayText: currentIndex === 0 ? qsTr("Select category...") : currentText
                    onActivated: idx => {
                        allocationModel.setProperty(index, "category", idx === 0 ? "" : root.categoryList[idx]);
                    }
                    Component.onCompleted: {
                        if (category === "") {
                            _unaffectedCategoryComboBox = categoryCombo;
                        }
                    }
                }

                AmountField {
                    id: allocationAmountField
                    Layout.preferredWidth: 120
                    value: amount
                    onEdited: newValue => {
                        allocationModel.setProperty(index, "amount", newValue);
                    }
                    onLiveEdited: newValue => {
                        allocationModel.setProperty(index, "amount", newValue);
                    }
                }

                ToolButton {
                    text: "⚖️"
                    focusPolicy: Qt.NoFocus
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Balance to remaining amount")
                    onClicked: {
                        // Add the remaining amount to this allocation's amount
                        let newAmount = amount + root.remainingAmount;
                        allocationModel.setProperty(index, "amount", newAmount);
                    }
                }

                ToolButton {
                    text: "🗑️"
                    enabled: allocationModel.count > 1
                    opacity: enabled ? 1.0 : 0.3
                    focusPolicy: Qt.NoFocus
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Remove category")
                    onClicked: root.removeAllocation(index)
                }
            }
        }

        // Add allocation button
        Button {
            Layout.alignment: Qt.AlignLeft
            text: qsTr("+ Add Category")
            onClicked: root.addAllocation()
        }

        RowLayout {
            Layout.fillWidth: true

            // Create rule button
            Button {
                text: qsTr("Create Rule...")
                visible: _operation !== null
                onClicked: {
                    if (_operation) {
                        ruleEditDialog.isNewRule = true;
                        ruleEditDialog.suggestedPrefix = labelField.text.trim();
                        ruleEditDialog.suggestedAmount = editedAmount;
                        if (allocationModel.count > 0 && allocationModel.get(0).category !== "") {
                            ruleEditDialog.suggestedCategory = allocationModel.get(0).category;
                        } else {
                            ruleEditDialog.suggestedCategory = "";
                        }
                        ruleEditDialog.open();
                    }
                }
            }
            Button {
                text: qsTr("Create counter part...")
                onClicked: counterPartDialog.open()
            }
        }
    }
}
