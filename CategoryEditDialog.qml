import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    title: qsTr("Edit Category")
    modal: true
    anchors.centerIn: parent
    width: 400
    standardButtons: Dialog.Ok | Dialog.Cancel

    property string originalName: ""
    property real originalBudgetLimit: 0  // Signed: positive = income, negative = expense

    onOpened: {
        categoryNameField.text = originalName;
        // Set checkbox based on sign (positive = income)
        incomeCheckBox.checked = originalBudgetLimit > 0;
        // Display absolute value
        budgetLimitField.value = Math.abs(originalBudgetLimit);
        budgetLimitField.forceActiveFocus();
    }

    onAccepted: {
        let amount = budgetLimitField.value;
        // Apply sign based on checkbox: income = positive, expense = negative
        let newBudgetLimit = incomeCheckBox.checked ? amount : -amount;
        budgetData.editCategory(originalName, categoryNameField.text, newBudgetLimit);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingNormal

        Label {
            text: qsTr("Name")
            font.pixelSize: Theme.fontSizeNormal
            color: Theme.textPrimary
        }

        TextField {
            id: categoryNameField
            Layout.fillWidth: true
            placeholderText: qsTr("Category name")
            font.pixelSize: Theme.fontSizeNormal
            onActiveFocusChanged: if (activeFocus)
                selectAll()
        }

        CheckBox {
            id: incomeCheckBox
            text: qsTr("This is an income category")
        }

        Label {
            text: qsTr("Budget Limit")
            font.pixelSize: Theme.fontSizeNormal
            color: Theme.textPrimary
        }

        AmountField {
            id: budgetLimitField
            Layout.fillWidth: true
            font.pixelSize: Theme.fontSizeNormal
        }
    }
}
