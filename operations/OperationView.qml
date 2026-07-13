import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import commonui

FocusScope {
    id: root
    objectName: "OperationView"

    required property var budgetData
    required property var categories
    required property var navigation
    required property var rules

    onActiveFocusChanged: {
        if (activeFocus) {
            operationList.forceActiveFocus();
        }
    }

    function addOperation() {
        operationEditDialog.initialize(null);
    }

    function editCurrentOperation() {
        operationEditDialog.initialize(budgetData.currentAccount.currentOperation);
    }

    RenameAccountDialog {
        id: renameDialog
        budgetData: root.budgetData
    }

    OperationEditDialog {
        id: operationEditDialog
        budgetData: root.budgetData
        categories: root.categories
        navigation: root.navigation
        rules: root.rules
        onClosed: operationList.forceActiveFocus()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingNormal

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal

            AccountComboBox {
                budgetData: root.budgetData
                Layout.preferredWidth: 200
                currentIndex: root.budgetData.currentAccountIndex
                onActivated: function (index) {
                    root.budgetData.currentAccountIndex = index;
                    operationList.forceActiveFocus();
                }
            }

            Button {
                text: qsTr("Rename")
                enabled: root.budgetData.accountCount > 0
                onClicked: renameDialog.open()
            }

            BalanceHeader {
                Layout.fillWidth: true
                balance: root.budgetData.currentAccount?.count > 0 ? root.budgetData.currentAccount?.balanceAt(0) : 0
                operationCount: root.budgetData.currentAccount?.count || 0
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingNormal

            OperationList {
                id: operationList
                account: root.budgetData.currentAccount
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            OperationDetails {
                id: operationDetails
                account: root.budgetData.currentAccount
                Layout.preferredWidth: parent.width * 0.3
                Layout.minimumWidth: 200
                Layout.maximumWidth: 400
                Layout.fillHeight: true
                operation: root.budgetData.currentAccount?.currentOperation
                onEditRequested: operation => {
                    operationEditDialog.initialize(operation);
                }
            }
        }
    }
}
