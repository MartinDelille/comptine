import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: importDialog
    title: qsTr("Import CSV")
    standardButtons: Dialog.Ok | Dialog.Cancel
    modal: true

    property string filePath: ""

    // Validation for new account name
    readonly property string newAccountName: newAccountField.text.trim()
    readonly property bool newAccountNameEmpty: newAccountRadio.checked && newAccountName === ""
    readonly property bool newAccountNameExists: {
        if (!newAccountRadio.checked || newAccountName === "")
            return false;
        return budgetData.getAccountByName(newAccountName) !== null;
    }
    readonly property bool isValid: existingAccountRadio.checked || (!newAccountNameEmpty && !newAccountNameExists)

    // Disable OK button when invalid
    onIsValidChanged: {
        standardButton(Dialog.Ok).enabled = isValid;
    }

    onOpened: {
        // Default to existing account if one is selected, otherwise new account
        if (budgetData.currentAccountIndex >= 0) {
            existingAccountRadio.checked = true;
            accountComboBox.currentIndex = budgetData.currentAccountIndex;
        } else {
            newAccountRadio.checked = true;
        }
        newAccountField.text = "";
        useCategoriesCheckBox.checked = false;
        // Update OK button state
        standardButton(Dialog.Ok).enabled = isValid;
    }

    onAccepted: {
        var accountName = "";
        if (newAccountRadio.checked) {
            accountName = newAccountName;
        } else {
            // Existing account
            var account = budgetData.getAccount(accountComboBox.currentIndex);
            accountName = account ? account.name : "";
        }

        budgetData.importFromCsv(filePath, accountName, useCategoriesCheckBox.checked);
        budgetData.currentTabIndex = 0;
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingNormal

        Label {
            text: qsTr("Import into account:")
        }

        ButtonGroup {
            id: accountButtonGroup
        }

        RadioButton {
            id: existingAccountRadio
            text: qsTr("Existing account")
            ButtonGroup.group: accountButtonGroup
            enabled: budgetData.accountCount > 0
        }

        ComboBox {
            id: accountComboBox
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingXLarge
            enabled: existingAccountRadio.checked
            model: budgetData.accountModel
            textRole: "name"
            delegate: ItemDelegate {
                required property int index
                required property string name
                width: accountComboBox.width
                text: name
                highlighted: accountComboBox.highlightedIndex === index
            }
        }

        RadioButton {
            id: newAccountRadio
            text: qsTr("New account")
            ButtonGroup.group: accountButtonGroup
        }

        TextField {
            id: newAccountField
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingXLarge
            enabled: newAccountRadio.checked
            placeholderText: qsTr("Account name")
        }

        Label {
            id: errorLabel
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingXLarge
            color: Theme.negative
            font.pixelSize: Theme.fontSizeSmall
            visible: newAccountRadio.checked && (newAccountNameEmpty || newAccountNameExists)
            text: {
                if (newAccountNameEmpty)
                    return qsTr("Account name is required");
                if (newAccountNameExists)
                    return qsTr("An account with this name already exists");
                return "";
            }
        }

        CheckBox {
            id: useCategoriesCheckBox
            text: qsTr("Use categories from CSV")
            checked: false
        }
    }
}
