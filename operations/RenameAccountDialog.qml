import QtQuick
import QtQuick.Controls

import commonui

BaseDialog {
    id: renameDialog
    title: qsTr("Rename Account")

    required property var budgetData
    property string originalName: ""
    okEnabled: renameField.text.trim() !== "" && renameField.text.trim() !== originalName

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

    Row {
        spacing: Theme.spacingNormal
        Label {
            text: qsTr("Account name:")
        }

        TextField {
            id: renameField
            placeholderText: qsTr("Enter account name")
            onActiveFocusChanged: {
                if (activeFocus)
                    selectAll();
            }
        }
    }
}
