import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

BaseDialog {
    id: root

    property int operationIndex: -1

    // Original values (for tracking changes)
    property double originalAmount: 0
    property date originalDate: new Date()
    property date originalBudgetDate: new Date()
    property string originalDescription: ""
    property string originalCategory: ""
    property var originalAllocations: []

    // Category list for ComboBoxes - refreshed on open
    property var categoryList: []

    title: qsTr("Edit Operation")
    width: Math.min(500, parent.width - 40)
    standardButtons: Dialog.Ok | Dialog.Cancel

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
    readonly property bool allCategoriesSelected: {
        if (allocationModel.count === 0)
            return false;
        for (let i = 0; i < allocationModel.count; i++) {
            if (allocationModel.get(i).category === "")
                return false;
        }
        return true;
    }
    readonly property bool hasDuplicateCategories: {
        let seen = new Set();
        for (let i = 0; i < allocationModel.count; i++) {
            let cat = allocationModel.get(i).category;
            if (cat !== "" && seen.has(cat))
                return true;
            seen.add(cat);
        }
        return false;
    }
    readonly property bool allocationsValid: Math.abs(remainingAmount) < 0.01 && allocationModel.count > 0 && allCategoriesSelected && !hasDuplicateCategories
    readonly property bool dateValid: dateDay.value >= 1 && dateDay.value <= 31
    readonly property bool budgetDateValid: budgetDateDay.value >= 1 && budgetDateDay.value <= 31
    readonly property bool isValid: allocationsValid && dateValid && budgetDateValid

    // Use BaseDialog's canSubmit for Enter key validation
    canSubmit: isValid

    // Disable OK button when invalid
    onOpened: {
        // Refresh category list when dialog opens
        root.categoryList = [""].concat(AppState.categories.categoryNames());

        let okButton = footer.standardButton(Dialog.Ok);
        if (okButton) {
            okButton.enabled = Qt.binding(function () {
                return root.isValid;
            });
        }
    }

    ListModel {
        id: allocationModel
    }

    function initialize(opIndex, amount, date, budgetDate, description, allocations, currentCategory) {
        operationIndex = opIndex;
        originalAmount = amount;
        editedAmount = amount;
        originalDate = date;
        originalBudgetDate = budgetDate;
        originalDescription = description;
        originalCategory = currentCategory;
        originalAllocations = allocations ? allocations.slice() : [];

        // Set description field
        descriptionField.text = description;

        // Set date spinboxes
        dateDay.value = date.getDate();
        dateMonth.currentIndex = date.getMonth();
        dateYear.value = date.getFullYear();

        // Set budget date spinboxes
        budgetDateDay.value = budgetDate.getDate();
        budgetDateMonth.currentIndex = budgetDate.getMonth();
        budgetDateYear.value = budgetDate.getFullYear();

        // Initialize allocations
        allocationModel.clear();
        if (allocations && allocations.length > 0) {
            // Existing split - load allocations
            for (let i = 0; i < allocations.length; i++) {
                allocationModel.append({
                    category: allocations[i].category,
                    amount: allocations[i].amount
                });
            }
        } else {
            // Single category - start with current category and full amount
            allocationModel.append({
                category: currentCategory ?? "",
                amount: amount
            });
        }
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

    onAccepted: {
        // Apply description change if different
        let newDescription = descriptionField.text.trim();
        if (newDescription !== originalDescription) {
            AppState.data.setOperationDescription(operationIndex, newDescription);
        }

        // Apply amount change if different
        if (Math.abs(editedAmount - originalAmount) > 0.001) {
            AppState.data.setOperationAmount(operationIndex, editedAmount);
        }

        // Apply budget date change if different
        let newBudgetDate = new Date(budgetDateYear.value, budgetDateMonth.currentIndex, budgetDateDay.value);
        if (newBudgetDate.getTime() !== originalBudgetDate.getTime()) {
            AppState.data.setOperationBudgetDate(operationIndex, newBudgetDate);
        }

        // Build allocations array and call splitOperation
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
        if (allocations.length > 0) {
            // Check if allocations changed
            let allocationsChanged = false;
            if (originalAllocations.length !== allocations.length) {
                allocationsChanged = true;
            } else if (originalAllocations.length === 0) {
                // Was single category, check if it changed
                if (allocations.length !== 1 || allocations[0].category !== originalCategory) {
                    allocationsChanged = true;
                }
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
                AppState.data.splitOperation(operationIndex, allocations);
            }
        }

        // Apply date change LAST (since it sorts and changes the operation's index)
        let newDate = new Date(dateYear.value, dateMonth.currentIndex, dateDay.value);
        if (newDate.getTime() !== originalDate.getTime()) {
            AppState.data.setOperationDate(operationIndex, newDate);
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingLarge

        // Description section
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal

            Label {
                text: qsTr("Description:")
                font.bold: true
                color: Theme.textSecondary
                Layout.preferredWidth: 100
            }

            TextField {
                id: descriptionField
                Layout.fillWidth: true
                placeholderText: qsTr("Enter description")
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

            DateSpinBox {
                id: dateDay
                from: 1
                to: 31
                value: 1
                Layout.preferredWidth: 80
            }

            ComboBox {
                id: dateMonth
                model: [qsTr("January"), qsTr("February"), qsTr("March"), qsTr("April"), qsTr("May"), qsTr("June"), qsTr("July"), qsTr("August"), qsTr("September"), qsTr("October"), qsTr("November"), qsTr("December")]
                Layout.fillWidth: true
            }

            DateSpinBox {
                id: dateYear
                from: 2000
                to: 2100
                value: 2024
                Layout.preferredWidth: 100
                textFromValue: function (value) {
                    return value.toString();
                }
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

            DateSpinBox {
                id: budgetDateDay
                from: 1
                to: 31
                value: 1
                Layout.preferredWidth: 80
            }

            ComboBox {
                id: budgetDateMonth
                model: [qsTr("January"), qsTr("February"), qsTr("March"), qsTr("April"), qsTr("May"), qsTr("June"), qsTr("July"), qsTr("August"), qsTr("September"), qsTr("October"), qsTr("November"), qsTr("December")]
                Layout.fillWidth: true
            }

            DateSpinBox {
                id: budgetDateYear
                from: 2000
                to: 2100
                value: 2024
                Layout.preferredWidth: 100
                textFromValue: function (value) {
                    return value.toString();
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.border
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

        // Validation message (always takes space to prevent dialog resize)
        Label {
            Layout.fillWidth: true
            opacity: !root.isValid && allocationModel.count > 0 ? 1.0 : 0.0
            text: {
                if (Math.abs(root.remainingAmount) >= 0.01)
                    return qsTr("Allocations must equal the total amount");
                if (!root.allCategoriesSelected)
                    return qsTr("All allocations must have a category");
                if (root.hasDuplicateCategories)
                    return qsTr("Each category can only be used once");
                return qsTr("At least one allocation is required");
            }
            color: Theme.warning
            font.pixelSize: Theme.fontSizeSmall
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
