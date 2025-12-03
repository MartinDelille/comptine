import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: importDialog
    title: qsTr("Import CSV")
    standardButtons: Dialog.Ok | Dialog.Cancel
    modal: true

    property string filePath: ""

    onOpened: {
        // Default to "New account" (last item in the list)
        accountComboBox.currentIndex = budgetData.accountCount;
    }

    onAccepted: {
        var accountName = "";
        // If not the last item ("New account"), use existing account name
        if (accountComboBox.currentIndex >= 0 && accountComboBox.currentIndex < budgetData.accountCount) {
            accountName = budgetData.getAccount(accountComboBox.currentIndex)?.name ?? "";
        }
        // Empty accountName will create a new account in importFromCsv
        budgetData.importFromCsv(filePath, accountName);
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
            // +1 for "New account" option at the end
            model: budgetData.accountCount + 1
            displayText: {
                if (currentIndex >= 0 && currentIndex < budgetData.accountCount) {
                    return budgetData.getAccount(currentIndex)?.name ?? qsTr("New account");
                }
                return qsTr("New account");
            }
            delegate: ItemDelegate {
                required property int index
                width: accountComboBox.width
                text: index < budgetData.accountCount ? (budgetData.getAccount(index)?.name ?? "") : qsTr("New account")
                highlighted: accountComboBox.highlightedIndex === index
            }
        }
    }
}
