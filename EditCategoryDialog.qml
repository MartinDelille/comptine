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

    // Parse amount string with French format support (e.g., "1 015,58 €")
    function parseAmount(str) {
        let cleaned = str.trim();
        // Remove spaces (thousand separators), non-breaking spaces, and Euro symbol
        cleaned = cleaned.replace(/[\s\u00A0\u202F€]/g, "");
        // Remove any sign - we'll apply it based on the checkbox
        cleaned = cleaned.replace(/^[+-]/, "");
        // French decimal comma to dot
        cleaned = cleaned.replace(",", ".");
        return parseFloat(cleaned) || 0;
    }

    onOpened: {
        categoryNameField.text = originalName;
        // Set checkbox based on sign (positive = income)
        incomeCheckBox.checked = originalBudgetLimit > 0;
        // Display absolute value
        budgetLimitField.text = Math.abs(originalBudgetLimit).toFixed(2);
        budgetLimitField.forceActiveFocus();
        budgetLimitField.selectAll();
    }

    onAccepted: {
        let amount = parseAmount(budgetLimitField.text);
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

        TextField {
            id: budgetLimitField
            Layout.fillWidth: true
            placeholderText: qsTr("0.00")
            font.pixelSize: Theme.fontSizeNormal
            onActiveFocusChanged: if (activeFocus)
                selectAll()
        }
    }
}
