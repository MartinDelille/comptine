import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import commonui

BaseDialog {
    id: root
    title: qsTr("Edit Category")
    width: 400

    required property var categories
    required property date date

    property var _category: null
    okEnabled: categoryNameField.text.trim().length > 0

    function edit(category = null) {
        _category = category;
        categoryNameField.text = _category ? _category.name : "";
        // Set checkbox based on sign (positive = income)
        let budgetLimit = _category ? _category.budgetLimitForMonth(date) : 0;
        incomeCheckBox.checked = budgetLimit > 0;
        // Display absolute value
        budgetLimitField.value = Math.abs(budgetLimit);
        if (_category) {
            budgetLimitField.forceActiveFocus();
        } else {
            categoryNameField.forceActiveFocus();
        }

        open();
    }

    onAccepted: {
        let amount = budgetLimitField.value;
        // Apply sign based on checkbox: income = positive, expense = negative
        let newBudgetLimit = incomeCheckBox.checked ? amount : -amount;
        categories.editCategory(categoryNameField.text, newBudgetLimit, _category, date);
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

        Label {
            text: qsTr("Budget Limit")
            font.pixelSize: Theme.fontSizeNormal
            color: Theme.textPrimary
        }

        AmountField {
            id: budgetLimitField
            Layout.fillWidth: true
            font.pixelSize: Theme.fontSizeNormal
            onEdited: function (newValue) {
                value = newValue;
            }
        }

        CheckBox {
            id: incomeCheckBox
            text: qsTr("This is an income category")
        }
    }
}
