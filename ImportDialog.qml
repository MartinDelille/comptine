import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

BaseDialog {
    id: importDialog
    title: qsTr("Import CSV")
    standardButtons: Dialog.Ok | Dialog.Cancel

    property string filePath: ""

    // Validation for new account name
    readonly property string newAccountName: newAccountField.text.trim()
    readonly property bool newAccountNameEmpty: newAccountRadio.checked && newAccountName === ""
    readonly property bool newAccountNameExists: {
        if (!newAccountRadio.checked || newAccountName === "")
            return false;
        return AppState.data.getAccountByName(newAccountName) !== null;
    }
    readonly property bool isValid: existingAccountRadio.checked || (!newAccountNameEmpty && !newAccountNameExists)

    // Use BaseDialog's canSubmit for Enter key validation
    canSubmit: isValid

    // Disable OK button when invalid
    onIsValidChanged: {
        standardButton(Dialog.Ok).enabled = isValid;
    }

    onOpened: {
        // Default to existing account if one is selected, otherwise new account
        if (AppState.navigation.currentAccountIndex >= 0) {
            existingAccountRadio.checked = true;
            accountComboBox.currentIndex = AppState.navigation.currentAccountIndex;
            accountComboBox.forceActiveFocus();
        } else {
            newAccountRadio.checked = true;
            newAccountField.forceActiveFocus();
        }
        // Extract filename without extension as default account name
        var fileName = filePath.substring(filePath.lastIndexOf("/") + 1);
        var baseName = fileName.substring(0, fileName.lastIndexOf("."));
        newAccountField.text = baseName || "";
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
            var account = AppState.data.getAccount(accountComboBox.currentIndex);
            accountName = account ? account.name : "";
        }

        AppState.file.importFromCsv(filePath, accountName, useCategoriesCheckBox.checked);
        AppState.navigation.currentTabIndex = 0;
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
            enabled: AppState.data.accountCount > 0
        }

        ComboBox {
            id: accountComboBox
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingXLarge
            enabled: existingAccountRadio.checked
            model: AppState.data.accountModel
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
