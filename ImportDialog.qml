import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: importDialog
    title: qsTr("Import CSV")
    standardButtons: Dialog.Ok | Dialog.Cancel
    modal: true

    property string filePath: ""

    // Build a local model with accounts + "New account" option
    ListModel {
        id: importAccountModel
    }

    function refreshAccountModel() {
        importAccountModel.clear();
        for (var i = 0; i < budgetData.accountCount; i++) {
            var account = budgetData.getAccount(i);
            importAccountModel.append({
                "name": account.name,
                "isNewAccount": false,
                "accountIndex": i
            });
        }
        importAccountModel.append({
            "name": qsTr("New account"),
            "isNewAccount": true,
            "accountIndex": -1
        });
    }

    onOpened: {
        refreshAccountModel();
        // Default to current account, or "New account" if none selected
        if (budgetData.currentAccountIndex >= 0 && budgetData.currentAccountIndex < budgetData.accountCount) {
            accountComboBox.currentIndex = budgetData.currentAccountIndex;
        } else {
            accountComboBox.currentIndex = importAccountModel.count - 1;
        }
        // Reset checkbox to unchecked
        useCategoriesCheckBox.checked = false;
    }

    onAccepted: {
        var selectedItem = importAccountModel.get(accountComboBox.currentIndex);
        var accountName = "";
        if (selectedItem && !selectedItem.isNewAccount) {
            var account = budgetData.getAccount(selectedItem.accountIndex);
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

        ComboBox {
            id: accountComboBox
            Layout.fillWidth: true
            Layout.preferredWidth: 250
            model: importAccountModel
            textRole: "name"
            delegate: ItemDelegate {
                required property int index
                required property string name
                width: accountComboBox.width
                text: name
                highlighted: accountComboBox.highlightedIndex === index
            }
        }

        CheckBox {
            id: useCategoriesCheckBox
            text: qsTr("Use categories from CSV")
            checked: false
        }
    }
}
