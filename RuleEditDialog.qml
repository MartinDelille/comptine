import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Comptine

BaseDialog {
    id: root
    title: isNewRule ? qsTr("Add Rule") : qsTr("Edit Rule")
    width: 450

    property bool isNewRule: true
    property int ruleIndex: -1
    property string originalCategory: ""
    property string originalLabelPrefix: ""
    property double originalAmountFilter: 0

    // For use when creating rule from OperationEditDialog
    property string suggestedPrefix: ""
    property string suggestedCategory: ""
    property double suggestedAmount: 0

    // Category list - refreshed on open
    property var categoryList: []

    okEnabled: categoryCombo.currentText.length > 0 && descriptionPrefixField.text.length > 0 && (!amountCheckBox.checked || amountFilterField.value !== 0)

    onOpened: {
        // Refresh category list when dialog opens
        root.categoryList = AppState.categories.categoryNames();

        if (isNewRule) {
            categoryCombo.currentIndex = -1;
            descriptionPrefixField.text = suggestedPrefix;
            if (suggestedCategory.length > 0) {
                let catIndex = categoryModel.findCategoryIndex(suggestedCategory);
                if (catIndex >= 0) {
                    categoryCombo.currentIndex = catIndex;
                }
            }
            // Pre-populate amount filter from suggested amount
            amountFilterField.value = suggestedAmount;
        } else {
            // Find and select the category
            let catIndex = categoryModel.findCategoryIndex(originalCategory);
            categoryCombo.currentIndex = catIndex;
            descriptionPrefixField.text = originalLabelPrefix;
            // Restore amount filter state
            if (originalAmountFilter !== 0) {
                amountCheckBox.checked = true;
                amountFilterField.value = originalAmountFilter;
            } else {
                amountCheckBox.checked = false;
                amountFilterField.value = 0;
            }
        }
        descriptionPrefixField.forceActiveFocus();
    }

    onAccepted: {
        let category = AppState.categories.getCategoryByName(categoryCombo.currentText);
        let prefix = descriptionPrefixField.text.trim();
        let amount = amountCheckBox.checked ? amountFilterField.value : 0;

        if (isNewRule) {
            AppState.rules.addRule(category, prefix, amount);
            if (applyToExistingCheckBox.checked) {
                let count = AppState.rules.applyRuleToUncategorized(category, prefix, amount);
                if (count > 0) {
                    console.log("Applied rule to", count, "uncategorized operation(s)");
                }
            }
        } else {
            AppState.rules.editRule(ruleIndex, category, prefix, amount);
        }
    }

    // Category model helper
    QtObject {
        id: categoryModel

        function findCategoryIndex(name) {
            let cats = root.categoryList;
            for (let i = 0; i < cats.length; i++) {
                if (cats[i] === name)
                    return i;
            }
            return -1;
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingNormal

        Label {
            text: qsTr("Label Prefix")
            font.pixelSize: Theme.fontSizeNormal
            color: Theme.textPrimary
        }

        TextField {
            id: descriptionPrefixField
            Layout.fillWidth: true
            placeholderText: qsTr("Operations starting with this text will match")
            font.pixelSize: Theme.fontSizeNormal
            onActiveFocusChanged: if (activeFocus)
                selectAll()
        }

        Label {
            text: qsTr("Assign Category")
            font.pixelSize: Theme.fontSizeNormal
            color: Theme.textPrimary
        }

        ComboBox {
            id: categoryCombo
            Layout.fillWidth: true
            model: root.categoryList
            font.pixelSize: Theme.fontSizeNormal
        }

        // Optional amount filter
        CheckBox {
            id: amountCheckBox
            text: qsTr("Match specific amount")
            font.pixelSize: Theme.fontSizeNormal
        }

        AmountField {
            id: amountFilterField
            enabled: amountCheckBox.checked
            Layout.fillWidth: true
            value: 0
        }

        // Show a hint about how rules work
        Label {
            Layout.fillWidth: true
            text: qsTr("Rules are matched in order. The first matching rule wins.")
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
        }

        // Option to apply the new rule to existing uncategorized operations
        CheckBox {
            id: applyToExistingCheckBox
            visible: root.isNewRule
            checked: true
            text: qsTr("Apply to existing uncategorized operations")
            font.pixelSize: Theme.fontSizeSmall
        }
    }
}
