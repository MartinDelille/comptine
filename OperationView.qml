import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: root
    objectName: "OperationView"

    // Forward focus to the operation list
    onActiveFocusChanged: {
        if (activeFocus) {
            operationList.forceActiveFocus();
        }
    }

    // Function to edit the current operation from menu action
    function editCurrentOperation() {
        if (operationList.currentIndex >= 0) {
            let op = AppState.data.operationModel.operationAt(operationList.currentIndex);
            if (op) {
                operationEditDialog.initialize(operationList.currentIndex, op.amount, op.date, op.budgetDate, op.description, op.isSplit ? op.allocations : [], op.category ?? "");
                operationEditDialog.open();
            }
        }
    }

    OperationEditDialog {
        id: operationEditDialog
        onClosed: operationList.forceActiveFocus()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingNormal

        Dialog {
            id: renameDialog
            title: qsTr("Rename Account")
            standardButtons: Dialog.Ok | Dialog.Cancel
            modal: true
            anchors.centerIn: parent

            property string originalName: ""

            onOpened: {
                originalName = AppState.data.currentAccount?.name ?? "";
                renameField.text = originalName;
                renameField.selectAll();
                renameField.forceActiveFocus();
            }

            onAccepted: {
                if (renameField.text.trim() !== "") {
                    AppState.data.renameCurrentAccount(renameField.text.trim());
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
                    onActiveFocusChanged: if (activeFocus)
                        selectAll()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal

            ComboBox {
                id: accountSelector
                Layout.preferredWidth: 200
                model: AppState.data.accountModel
                textRole: "name"
                currentIndex: AppState.navigation.currentAccountIndex
                enabled: AppState.data.accountCount > 0
                displayText: AppState.data.currentAccount?.name ?? qsTr("No account")
                onActivated: function (index) {
                    AppState.navigation.currentAccountIndex = index;
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
                enabled: AppState.data.accountCount > 0
                onClicked: renameDialog.open()
            }

            BalanceHeader {
                Layout.fillWidth: true
                balance: AppState.data.operationModel.count > 0 ? AppState.data.operationModel.balanceAt(0) : 0
                operationCount: AppState.data.operationModel.count
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
                id: operationDetails
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                currentIndex: operationList.currentIndex
                onEditRequested: (operationIndex, amount, operationDate, budgetDate, description, allocations, currentCategory) => {
                    operationEditDialog.initialize(operationIndex, amount, operationDate, budgetDate, description, allocations, currentCategory);
                    operationEditDialog.open();
                }
            }
        }
    }
}
