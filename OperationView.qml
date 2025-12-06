import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root
    objectName: "OperationView"
    spacing: Theme.spacingNormal

    Dialog {
        id: renameDialog
        title: qsTr("Rename Account")
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        anchors.centerIn: parent

        property string originalName: ""

        onOpened: {
            originalName = budgetData.currentAccount?.name ?? "";
            renameField.text = originalName;
            renameField.selectAll();
            renameField.forceActiveFocus();
        }

        onAccepted: {
            if (renameField.text.trim() !== "") {
                budgetData.renameCurrentAccount(renameField.text.trim());
            }
        }

        ColumnLayout {
            spacing: Theme.spacingNormal

            Label {
                text: qsTr("Account name:")
            }

            TextField {
                id: renameField
                Layout.preferredWidth: 250
                placeholderText: qsTr("Enter account name")
                onAccepted: renameDialog.accept()
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.spacingNormal

        ComboBox {
            id: accountSelector
            Layout.preferredWidth: 200
            model: budgetData.accountModel
            textRole: "name"
            currentIndex: budgetData.currentAccountIndex
            enabled: budgetData.accountCount > 0
            displayText: budgetData.currentAccount?.name ?? qsTr("No account")
            onActivated: function(index) {
                budgetData.currentAccountIndex = index;
            }
            delegate: ItemDelegate {
                required property int index
                required property string name
                width: accountSelector.width
                text: name
                highlighted: accountSelector.highlightedIndex === index
            }
        }

        Button {
            text: qsTr("Rename")
            enabled: budgetData.accountCount > 0
            onClicked: renameDialog.open()
        }

        BalanceHeader {
            Layout.fillWidth: true
            balance: budgetData.operationModel.count > 0 ? budgetData.operationModel.data(budgetData.operationModel.index(0, 0), 261) : 0
            operationCount: budgetData.operationModel.count
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: Theme.spacingNormal

        OperationList {
            id: operationList
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        OperationDetails {
            Layout.preferredWidth: 300
            Layout.fillHeight: true
            currentIndex: operationList.currentIndex
        }
    }
}
