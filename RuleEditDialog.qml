import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

BaseDialog {
    id: root
    title: isNewRule ? qsTr("Add Rule") : qsTr("Edit Rule")
    width: 450
    standardButtons: Dialog.Ok | Dialog.Cancel

    property bool isNewRule: true
    property int ruleIndex: -1
    property string originalCategory: ""
    property string originalDescriptionPrefix: ""

    // For use when creating rule from CategorizeDialog
    property string suggestedPrefix: ""

    // Category list - refreshed on open
    property var categoryList: []

    canSubmit: categoryCombo.currentText.length > 0 && descriptionPrefixField.text.length > 0

    onOpened: {
        // Refresh category list when dialog opens
        root.categoryList = AppState.categories.categoryNames();

        if (isNewRule) {
            categoryCombo.currentIndex = -1;
            descriptionPrefixField.text = suggestedPrefix;
        } else {
            // Find and select the category
            let catIndex = categoryModel.findCategoryIndex(originalCategory);
            categoryCombo.currentIndex = catIndex;
            descriptionPrefixField.text = originalDescriptionPrefix;
        }
        descriptionPrefixField.forceActiveFocus();
    }

    onAccepted: {
        let category = AppState.categories.getCategoryByName(categoryCombo.currentText);
        let prefix = descriptionPrefixField.text.trim();

        if (isNewRule) {
            AppState.rules.addRule(category, prefix);
        } else {
            AppState.rules.editRule(ruleIndex, category, prefix);
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
            text: qsTr("Description Prefix")
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

        // Show a hint about how rules work
        Label {
            Layout.fillWidth: true
            text: qsTr("Rules are matched in order. The first matching rule wins.")
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
        }
    }
}
